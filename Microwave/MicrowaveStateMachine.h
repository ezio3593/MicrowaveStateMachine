#include "stdafx.h"
#include <map>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

class CallbackFunc
{
	public:
		virtual void operator()() = 0;
};

template<class E, class S, class T> class Transitions
{
	typedef void (T::* ACTION)();
	S state;
	ACTION action;

	public:
		Transitions(S _state, ACTION _action): state(_state), action(_action) {}
		S getNextState() {return state;}
		ACTION getAction() {return action;}
};

template<class F> class Timer
{
	std::mutex m;
	std::condition_variable cv;
	unsigned int time;
	bool isStop;
	F* func;

	void threadFunction(F* func);

	public:
		Timer(F* _func);
		void start();
		void stop();
		void set(unsigned int _time) {time = _time;}
		~Timer() {}
};

class ImpossibleEventException: public std::exception
{
	public:
		ImpossibleEventException() : exception() {}
};

class MicrowaveStateMachine
{
	enum event
	{
		SET_TIMER,
		OPEN_DOOR,
		CLOSE_DOOR,
		START,
		TIME_OUT
	};

	enum state
	{
		IDLE_OPEN_DOOR,
		IDLE_CLOSE_DOOR,
		TIMER_OPEN_DOOR,
		TIMER_CLOSE_DOOR,
		COOKING
	};
	
	class TransMapKey
	{
		state st;
		event ev;
		public:
			TransMapKey(state s, event e): st(s), ev(e) {}
			friend bool operator<(const TransMapKey& key, const TransMapKey& key1) {return (key.st < key1.st) || (key.st == key1.st) && (key.ev < key1.ev);}
			~TransMapKey() {}
 	};


	template<class T, class Arg> class CallbackTimer
	{
		typedef void (T::* F)(Arg);
		T* obj;
		Arg arg;
		F f;
		std::mutex& m;

		public:
			CallbackTimer(T* _obj, std::mutex& _m, F _f, Arg _arg): obj(_obj), f(_f), arg(_arg), m(_m) {}
			void operator()();
			~CallbackTimer() {}
	};

	typedef Transitions<event, state, MicrowaveStateMachine> Trans;

	std::atomic_uchar timerValue;
	state currentState;
	Timer<CallbackTimer<MicrowaveStateMachine, event>>* timer;
	CallbackTimer<MicrowaveStateMachine, event> *callback;
	std::map<TransMapKey, Trans> transMap;
	CallbackFunc* callbackFunc;
	std::mutex m;

	void handleEvent(event e);
	void setTimerAction() {timer->set(timerValue);}
	void startAction() {timer->start();}
	void interruptCookingAction() {timerValue = 0; timer->set(timerValue); timer->stop();}
	void endCookingAction() {timerValue = 0; timer->set(timerValue); if(callbackFunc) (*callbackFunc)();}

	public:

		void openDoor();
		void closeDoor();
		void setTimer(unsigned char timer);
		void start();
		void setCallbackFunc(CallbackFunc* func);
		std::wstring getCurrentStateName();
		std::atomic_uchar getTimerValue() {return timerValue;}
		MicrowaveStateMachine();
		~MicrowaveStateMachine();
};