#include "stdafx.h"
#include "MicrowaveStateMachine.h"

MicrowaveStateMachine::MicrowaveStateMachine()
{typedef void (MicrowaveStateMachine::* ACTION)(void);
	timerValue = 0;
	callbackFunc = NULL;
	currentState = IDLE_CLOSE_DOOR;
	callback = new CallbackTimer<MicrowaveStateMachine, event>(this, m, &MicrowaveStateMachine::handleEvent, TIME_OUT);
	timer = new Timer<CallbackTimer<MicrowaveStateMachine, event>>(callback);

	transMap.insert(std::pair<TransMapKey, Trans>(TransMapKey(IDLE_CLOSE_DOOR, OPEN_DOOR), Trans(IDLE_OPEN_DOOR, NULL)));
	transMap.insert(std::pair<TransMapKey, Trans>(TransMapKey(IDLE_OPEN_DOOR, CLOSE_DOOR), Trans(IDLE_CLOSE_DOOR, NULL)));

	transMap.insert(std::pair<TransMapKey, Trans>(TransMapKey(IDLE_CLOSE_DOOR, SET_TIMER), Trans(TIMER_CLOSE_DOOR, setTimerAction)));
	transMap.insert(std::pair<TransMapKey, Trans>(TransMapKey(IDLE_OPEN_DOOR, SET_TIMER), Trans(TIMER_OPEN_DOOR, setTimerAction)));

	transMap.insert(std::pair<TransMapKey, Trans>(TransMapKey(TIMER_CLOSE_DOOR, SET_TIMER), Trans(TIMER_CLOSE_DOOR, setTimerAction)));
	transMap.insert(std::pair<TransMapKey, Trans>(TransMapKey(TIMER_OPEN_DOOR, SET_TIMER), Trans(TIMER_OPEN_DOOR, setTimerAction)));

	transMap.insert(std::pair<TransMapKey, Trans>(TransMapKey(TIMER_CLOSE_DOOR, OPEN_DOOR), Trans(TIMER_OPEN_DOOR, NULL)));
	transMap.insert(std::pair<TransMapKey, Trans>(TransMapKey(TIMER_OPEN_DOOR, CLOSE_DOOR), Trans(TIMER_CLOSE_DOOR, NULL)));

	transMap.insert(std::pair<TransMapKey, Trans>(TransMapKey(TIMER_CLOSE_DOOR, START), Trans(COOKING, &MicrowaveStateMachine::startAction)));
	transMap.insert(std::pair<TransMapKey, Trans>(TransMapKey(COOKING, OPEN_DOOR), Trans(TIMER_OPEN_DOOR, &MicrowaveStateMachine::interruptCookingAction)));
	transMap.insert(std::pair<TransMapKey, Trans>(TransMapKey(COOKING, SET_TIMER), Trans(TIMER_CLOSE_DOOR, &MicrowaveStateMachine::interruptCookingAction)));

	transMap.insert(std::pair<TransMapKey, Trans>(TransMapKey(COOKING, TIME_OUT), Trans(IDLE_CLOSE_DOOR, &MicrowaveStateMachine::endCookingAction)));
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