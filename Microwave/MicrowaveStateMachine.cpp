#include "stdafx.h"
#include "MicrowaveStateMachine.h"

typedef void (MicrowaveStateMachine::* ACTION)();

MicrowaveStateMachine::MicrowaveStateMachine()
{
	timerValue = 0;
	callbackFunc = NULL;
	currentState = IDLE_CLOSE_DOOR; 
	callback = new CallbackTimer<MicrowaveStateMachine, event>(this, m, &MicrowaveStateMachine::handleEvent, TIME_OUT);
	timer = new Timer<CallbackTimer<MicrowaveStateMachine, event>>(callback);
	
	addTransition(IDLE_CLOSE_DOOR, OPEN_DOOR, IDLE_OPEN_DOOR, NULL);
	addTransition(IDLE_OPEN_DOOR, CLOSE_DOOR, IDLE_CLOSE_DOOR, NULL);

	addTransition(IDLE_CLOSE_DOOR, SET_TIMER, TIMER_CLOSE_DOOR, getAction(SET_TIMER_ACTION));
	addTransition(IDLE_OPEN_DOOR, SET_TIMER, TIMER_OPEN_DOOR, getAction(SET_TIMER_ACTION));

	addTransition(TIMER_CLOSE_DOOR, SET_TIMER, TIMER_CLOSE_DOOR, getAction(SET_TIMER_ACTION));
	addTransition(TIMER_OPEN_DOOR, SET_TIMER, TIMER_OPEN_DOOR, getAction(SET_TIMER_ACTION));

	addTransition(TIMER_CLOSE_DOOR, OPEN_DOOR, TIMER_OPEN_DOOR, NULL);
	addTransition(TIMER_OPEN_DOOR, CLOSE_DOOR, TIMER_CLOSE_DOOR, NULL);

	addTransition(TIMER_CLOSE_DOOR, START, COOKING, getAction(START_ACTION));
	addTransition(COOKING, OPEN_DOOR, TIMER_OPEN_DOOR, getAction(INTERRUPT_COOKING_ACTION));
	addTransition(COOKING, SET_TIMER, TIMER_CLOSE_DOOR, getAction(INTERRUPT_COOKING_ACTION));

	addTransition(COOKING, TIME_OUT, IDLE_CLOSE_DOOR, getAction(END_COOKING_ACTION));
}

void MicrowaveStateMachine::addTransition(state s, event e, state nextState, ACTION action)
{
	transMap.insert(std::pair<TransMapKey, Trans>(TransMapKey(s, e), Trans(nextState, action)));
}

ACTION MicrowaveStateMachine::getAction(action act)
{
	switch (act)
	{
		case SET_TIMER_ACTION:
			return &MicrowaveStateMachine::setTimerAction;
		case START_ACTION:
			return &MicrowaveStateMachine::startAction;
		case INTERRUPT_COOKING_ACTION:
			return &MicrowaveStateMachine::interruptCookingAction;
		case END_COOKING_ACTION:
			return &MicrowaveStateMachine::endCookingAction;
	}
}

void MicrowaveStateMachine::handleEvent(event e)
{
	try
	{
		Trans tr = transMap.at(TransMapKey(currentState, e));
		currentState = tr.getNextState();
		if(tr.getAction()) (this->*tr.getAction())();
	} catch (const std::out_of_range&) {throw ImpossibleEventException();};
}

void MicrowaveStateMachine::openDoor()
{
	std::lock_guard<std::mutex> lk(m);
	handleEvent(OPEN_DOOR);
}

void MicrowaveStateMachine::closeDoor() 
{
	std::lock_guard<std::mutex> lk(m);
	handleEvent(CLOSE_DOOR);
}

void MicrowaveStateMachine::setTimer(unsigned char timer)
{
	std::lock_guard<std::mutex> lk(m);
	timerValue = timer;
	handleEvent(SET_TIMER);
}

void MicrowaveStateMachine::start() 
{
	std::lock_guard<std::mutex> lk(m);
	handleEvent(START);
}

std::wstring MicrowaveStateMachine::getCurrentStateName()
{
	switch (currentState)
	{
		case IDLE_CLOSE_DOOR:
			return L"IDLE_CLOSE_DOOR";
		case IDLE_OPEN_DOOR:
			return L"IDLE_OPEN_DOOR";
		case TIMER_OPEN_DOOR:
			return L"TIMER_OPEN_DOOR";
		case TIMER_CLOSE_DOOR:
			return L"TIMER_CLOSE_DOOR";
		case COOKING:
			return L"COOKING";
	}
}

void MicrowaveStateMachine::setCallbackFunc(CallbackFunc* func)
{
	std::lock_guard<std::mutex> lk(m);
	callbackFunc = func;
}

MicrowaveStateMachine::~MicrowaveStateMachine()
{
	std::lock_guard<std::mutex> lk(m);
	transMap.clear();
	delete callback;
	delete timer;
}

template<class T, class Arg> void MicrowaveStateMachine::CallbackTimer<T, Arg>::operator()()
{
	std::lock_guard<std::mutex> lk(m);
	(obj->*f)(arg);
}

template<class Func> Timer<Func>::Timer(Func* _func)
{
	func = _func;
	isStop = false;
	time = 0;
}

template<class Func> void Timer<Func>::start() 
{
	isStop = false;
	std::thread thread(&Timer::threadFunction, this, func);
	thread.detach();
}

template<class Func> void Timer<Func>::stop() 
{ 
	std::lock_guard<std::mutex> lk(m);
	isStop = true;
	cv.notify_one();
}

template<class Func> void Timer<Func>::threadFunction(Func *func) 
{
	std::unique_lock<std::mutex> lk(m);
	cv.wait_for(lk, std::chrono::seconds(time), [this]{return this->isStop;});
	
	if (!isStop && func != NULL) (*func)();
};