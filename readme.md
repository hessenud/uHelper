This is a very basic C++ library for Arduino, implementing some Helper classes i repeatedly 
needed for simple Arduino experiments.
Because i took the ntp-client from Francesco Potortì which is under GPLv3 this library is also 
entierely under GPLv3.


Installation
--------------------------------------------------------------------------------

To install this library, just place this entire folder as a subfolder in your
Arduino/lib/targets/libraries folder.

When installed, this library should look like:

Arduino/lib/targets/libraries/uHelper              (this library's folder)
Arduino/lib/targets/libraries/uHelper/uHelper.cpp  (the library implementation file)
Arduino/lib/targets/libraries/uHelper/uHelper.h    (the library description file)
Arduino/lib/targets/libraries/uHelper/keywords.txt (the syntax coloring file)
Arduino/lib/targets/libraries/uHelper/examples     (the examples in the "open" menu)
Arduino/lib/targets/libraries/uHelper/readme.txt   (this file)

After this library is installed, you just have to start the Arduino application.
You may see a few warning messages as it's built.

To use this library in a sketch, go to the Sketch | Import Library menu and
select uHelper.  This will add a corresponding line to the top of your sketch:

\#include <uHelper.h>

To stop using this library, delete that line from your sketch.
 

  Helper classes
--------------------------------------------------------------------------------

TimeClk
=======
NTP synchronized clock with 1s resolution. The NTP client ist taken from Francesco Potortì
	 
To start a TimeClk instance:
	
	void begin(int i_tz /*offs in 100th hrs*/ , unsigned  i_locPort = 2390);
	TimeClk tClk;
	tClk.begin( +100 /*CET*/ ); // start ntpClient for CET on standard port
	
Read the local time	

	unsigned long read();
	tClk.read();
	
	
Some timestring converters

	static const char* getTimeString( unsigned long theTime );  	// convert theTime to c-str "hh:mm:ss"
	static const char* getTimeStringS( unsigned long theTime ); 	// convert theTime to short c-str "hh:mm"
	static const char* getTimeStringDays( unsigned long theTime );	// convert days part of theTime to c-str "dd:>"


Convert a daytime like 0800 to a unix time (seconds since 1970...)

	static unsigned long daytime2unixtime(unsigned long i_daytime, unsigned long _now);
	
	
uTimer
=======
Timers...  this is raw experimental stuff.. so look in the header for documentation...


	uTimer* timerlist;
	TimeClk  clk;

  	clk.begin( i_timeZone ); 
  	timerlist = new uTimer( TimeClk::daytime2unixtime(8 Hrs, getTime()) /*0800h*/, 1 DAY, true, 0, callBackFunktion );


	void loopTimeClk()
	{
	  static unsigned long _last;
	  unsigned long _time = clk.read(); 
	  if ( _last != _time ) {
	    _last = _time;
	    for( uTimer* wp = g_Timerlist; wp; wp = wp->next() ) {
	      wp->check(_time);
	    }
	  }
	}
	

Debounce
=======
Digital inputs like switches need to be filtered to avoid spurious signals, and bouncing inputs.
An instance of _Debounce_ can be used to realize this simple filter:

	Debounce    fltr( HIGH , 50);   // input is iniially HIGH, 50ms debounce delay 
	fltr.set( HIGH );  				// manually set input state 		
	
	int filtered_in =  fltr.get( digitalRead(inputPin) ); // read and filter dig-in
	

DebouncedInput
=======
Like Debounce but specialized for digital input:
	
	DebouncedInput    key( inputPin , 50);   //  50ms debounce delay 
	
	int keyState =    key.read();  // filtered input-level from inputPin
	

PushButton
=======
_PushButton_ is a debounced input, that delivers interpreted input _events_:
* CLICK	    // BUtton click DN - UP
* UP		// Button was released
* DN		// Button is pressed
* DN_LONG1  // Button is held longer than 1st Threshold
* DN_LONG2 	// Button is held longer than 2nd Threshold
* DBLCLICK 	
* TRIPLECLICK 
* QUADCLICK

	PushButton( int i_input, unsigned i_debounceDelay = 50,
			unsigned long i_long1_threshold = 3000,
			unsigned long i_long2_threshold = 8000,
			unsigned long i_multiclick_threshold= 400 // repeated clicks within this time are considered multiclicks
			)


BlinkSignal
=======
The first thing you get up running on a new board is often the Blink example. Like "hello, world" without Display or serial interface.
So signalling by using a LED and blink patterns is a useful thing.

_BlinkSignal_ is a class for signalling status/events by various blink patterns.   

	BlinkSignal(unsigned i_ticklen= 250, unsigned i_idle = 0x0f0f0f0f ); // default 250ms ticklen, idle pattern 4ticks on 4 ticks off f=0.5Hz
	void queue( unsigned i_signal, unsigned i_len=DEFAULT_SIGLEN, bool i_repeat=false); // set next pattern, finish active pattern 
	void set( unsigned i_signal, unsigned i_len=DEFAULT_SIGLEN, bool i_repeat=false);	// abort active pattern an set new pattern immediately
	void reset();
	void setIdle( unsigned i_signal, unsigned i_len=DEFAULT_SIGLEN)
	bool tick();	// return the LED state 
	
	BlinkSignal  sig()  // 1 Tick 250ms  BlinkPattern
	sig.setIdle( 1, 2 ) // new blink pattern 0b01  len = 2 Bit blink f=2Hz
	
	sig.set( 0b1010111, 8 ) // long short short blink total len = 8 Ticks
	sig.next( 0b101011101, 20) // short long short short long pause total len = 20 ticks
		// after these two signals the BlinkSignal switches back to the idle pattern
    
	[...in mainLoop...]
		digitalWrite( sig.tick() ); // tick timing is realized inside sig
	[...]
	

MorseCoder
=======
_MorseCoder_ is a specialized BlinkSignal. ...obviously for Morse code.

	MorseCoder(unsigned i_ticklen= 250;

	void next( const char* i_msg, bool i_repeat = false);
	void set( const char* i_msg, bool i_repeat = false);	
	void reset();
	bool is_active();
	bool is_idle();
	bool tick();
	
	MorseCoder  beeper(200);  // approx. 60sym/s
	
	beeper.next("cq cq de Arduino pse k");
	beeper.next("-.-. --.- -.. . -- ."); //  raw morse  cq de me

