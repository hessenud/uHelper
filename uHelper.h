#ifndef _uHELPER_H_
#define _uHELPER_H_
#include <Arduino.h>
#include "pgmspace.h"
#include <WiFiUDP.h>






//--------------------------------------------
//--- Aux Helper  ------------------------
//--------------------------------------------

/**
 * @brief make a member (const) readable
 */
#define attr_reader( __t, __v ) const __t & __v=m_##__v

const char* storeString( String istr );
const char* replaceString( char** io_buf, String istr );

#ifndef DIM
# define DIM(x) (sizeof(x)/sizeof(x[0]))
#endif

//--------------------------------------------
//--- Time Helper  ------------------------
//--------------------------------------------

#define Min *60
#define Hrs *(60 Min)
#define Day *(24 Hrs)


#define mkDelegate( __obj, __meth )  [](){ (__obj)->__meth(); }

class uTimer; // forward
class uTimer {
    unsigned long m_swTime;     ///< switch time in minutes
    unsigned long m_interval;     ///< switch time in minutes
    bool          m_switchmode; ///< true = on false = off
    int*          m_channel;    ///< switch number
    void          (*m_channel_cb)(bool i_switchmode); ///<  call a delegate if any
    bool          m_hot;        //< time != swTime; hot<-true | when swTime is reached timer is executed and hot <- false

    uTimer*       m_next;
public:
    uTimer(unsigned long i_swTime, unsigned long i_interval=0, bool i_switchmode=true, int* i_channel =0, void (*i_channel_cb)(bool)=0):
        m_swTime(i_swTime), m_interval(i_interval), m_switchmode(i_switchmode),m_channel(i_channel),m_channel_cb(i_channel_cb),m_hot(true),m_next(0){}

    unsigned long getSwTime() { return m_swTime; }
    bool          hot() { return m_hot; }
    uTimer*       next(){ return m_next; }
    void          trigger();
    void          append(uTimer* i_tmt);
    void          append_to(uTimer* i_list);
    bool          check(unsigned long _time);

    /** re-arm the timer by adding an interval to last trigger time
     *  the interval is applied until next trigger lies in the future,
     *  the standard interval is 0s = no repetition   // for repetition 1DAY == 24*3600s
     *
     *  @param i_interval  if nonzero advance trigger by i_interval seconds else use m_interval
     */
    void rearm(unsigned long i_interval=1 Day) {
        m_hot = true;
        if ( i_interval ) {
            m_interval = i_interval;
        }
        m_swTime +=  m_interval;
    }

    /**
     * set a uTimer
     * @param i_swTime
     * @param i_interval    repetition time 0 => sigle shot
     * @param i_switchmode  ON=true OFF=false
     * @param i_channel     e.g. PIN number
     * @param i_channel_cb  callback function on trigger
     */
    void set(unsigned long i_swTime, unsigned long i_interval=0, bool i_switchmode=true, int* i_channel =0, void (*i_channel_cb)(bool)=0)
    {
        m_swTime = i_swTime;
        m_interval = i_interval;
        m_switchmode = i_switchmode;
        m_channel = i_channel;
        m_channel_cb = i_channel_cb;
        m_hot = true;
    }


    ///  disarm timer
    void disarm() { m_hot = false; }

    /// @return true if timer is repetitive
    bool is_repetitive() { return (m_swTime!=0) && (m_interval!=0); }

    /// @return true if timer is repetitive
    bool is_active() { return m_swTime!=0; }

};




//--------------------------------------------
//--- NTP SystemTime  ------------------------
//--------------------------------------------


/********************************************************************
 * TimeClk   with NTP Client
 ********************************************************************/
class TimeClk {
    int  m_tz;
    unsigned int  m_localPort;     //<  local port to listen for UDP packets
    unsigned long m_localTime;	 ///< local unix-time  (seconds passed since...)
    bool			m_dst;			 ///< true = daylight saving time
    unsigned 		m_ltime_ms;		 ///< local time in milliseconds
    unsigned long m_ntpTime;		 ///< last ntp time
    int 			m_tickLen;		 ///< ticklen of ntp time (1s) in local milliseconds (clock discipline)

    // A UDP instance to let us send and receive packets over UDP
    WiFiUDP m_udp;
    // send an NTP request to the time server at the given address



    unsigned long ntpUnixTime();

    unsigned long utc2local(unsigned long utc)
    {
        unsigned long tz = m_tz; //100th hours
        return utc + (36 * tz);
    }

public:


    TimeClk( ):m_tz(0),m_localPort(0),m_ltime_ms(0),m_ntpTime(0), m_tickLen(1000){
    }

    void begin(int i_tz , unsigned  i_locPort = 2390) {
        m_tz = i_tz;
        m_localPort = i_locPort;
        m_udp.begin(m_localPort);
    }

    unsigned long read();

    static const char* getTimeString( unsigned long theTime );
    static const char* getTimeStringS( unsigned long theTime );
    static const char* getTimeStringDays( unsigned long theTime );

    /**
     * convert a day-local offset to unixtime
     * @param
     */
    static unsigned long daytime2unixtime( unsigned long i_daytime, unsigned long _now);

    /**
     * @return timestring as numeric value in seconds
     */
    static unsigned long timeStr2Value(  const char* i_ts);

};


//--------------------------------------------
//--- IO / Key Debouncer / Clickfilert  ------
//--------------------------------------------


class Debounce {
protected:
    bool m_state;   // the current reading from the input pin
    bool m_lastState;   // the previous reading from the input pin

    // the following variables are unsigned longs because the time, measured in
    // milliseconds, will quickly become a bigger number than can be stored in an int.
    unsigned long m_lastDebounceTime;  // the last time the output pin was toggled
    unsigned long m_debounceDelay;    // the debounce time; increase if the output flickers


public:

    Debounce(bool i_initial, unsigned i_debounceDelay=50):m_state(i_initial),m_lastState(i_initial),m_lastDebounceTime(0),m_debounceDelay(i_debounceDelay) {
    }

    void set(bool i_state) {
        m_state = m_lastState = i_state;
    }
    bool get( int input) {
        // check to see if you just pressed the button
        // (i.e. the input went from LOW to HIGH), and you've waited long enough
        // since the last press to ignore any noise:

        // If the switch changed, due to noise or pressing:
        if (input != m_lastState) {
            // reset the debouncing timer
            m_lastDebounceTime = millis();
        }

        if ((millis() - m_lastDebounceTime) > m_debounceDelay) {
            // whatever the reading is at, it's been there for longer than the debounce
            // delay, so take it as the actual current state:

            // if the button state has changed:
            if (input != m_state) {
                m_state = input;
            }
        }
        // save the reading. Next time through the loop, it'll be the lastButtonState:
        m_lastState = input;
        return  m_state;
    }
};

class DebouncedInput:public Debounce {
    int m_input;
public:

    DebouncedInput(const int i_inputPin, unsigned i_delay=50):Debounce( digitalRead( i_inputPin ), i_delay), m_input(i_inputPin) {
    }

    int read() {
        // check to see if you just pressed the button
        // (i.e. the input went from LOW to HIGH), and you've waited long enough
        // since the last press to ignore any noise:
        return get( digitalRead( m_input ));
    }

    bool state() {
        return m_state == 0; // LOW means button pressed
    }
};


class PushButton: public DebouncedInput {

    unsigned long m_dnTime;
    unsigned long m_upTime;
    int m_btn_state;
    unsigned m_clicks;
    unsigned long m_long1_threshold = 3000;   // 1s
    unsigned long m_long2_threshold = 8000;   // 6s
    unsigned long m_multiclick_threshold = 400; // 400ms
public:
    enum { // Events
        IDLE = 0,
        CLICK,  // DN - UP
        UP,
        DN,
        DN_LONG1,
        DN_LONG2,
        DBLCLICK,
        TRIPLECLICK,
        QUADCLICK,
    };
    enum { //STATES
        sIDLE = 0,
        sUP,
        sUP_1,
        sDN,
        sDN_LONG1,
        sDN_LONG2,
    };
    PushButton( int i_input, unsigned i_debounceDelay = 50,
            unsigned long i_long1_threshold = 3000,
            unsigned long i_long2_threshold = 8000,
            unsigned long i_multiclick_threshold= 400 ):
                DebouncedInput(i_input, i_debounceDelay), m_btn_state(sUP), m_clicks(0),
                m_long1_threshold(i_long1_threshold), m_long2_threshold(i_long2_threshold),
                m_multiclick_threshold(i_multiclick_threshold) {

    }
    int getEvent();
};


//--------------------------------------------
//--- LED Signaler --------------------------
//--------------------------------------------

class BlinkSignal {
    bool     m_idle;
    unsigned m_idle_signal;
    unsigned m_idle_len;
    unsigned m_signal;
    unsigned m_len;
    unsigned m_next_signal;
    unsigned m_next_len;
    bool     m_signal_pending;
    bool	   m_signal_repeat; ///< true -> reload next_signal
    unsigned long m_lastTick;
    unsigned m_tickNr;
    unsigned m_ticklen;
    static const unsigned DEFAULT_SIGLEN = sizeof(m_signal)*8;

public:

    BlinkSignal(unsigned i_ticklen= 250, unsigned i_idle = 0x0f0f0f0f):m_idle(false),
    m_idle_signal(i_idle),m_signal(0),m_len(0),
    m_next_signal(0), m_next_len(0), m_signal_pending(false),
    m_lastTick(0) , m_tickNr(0),m_ticklen( i_ticklen )
    {}

    void next( unsigned i_signal, unsigned i_len=DEFAULT_SIGLEN, bool i_repeat=false){
        m_next_signal = i_signal;
        m_next_len = i_len;
        m_signal_pending = true;
        m_signal_repeat = i_repeat;
        if ( m_idle ) {m_idle = false; m_tickNr = m_len; }// enforce reload
    }
    void set( unsigned i_signal, unsigned i_len=DEFAULT_SIGLEN, bool i_repeat=false){
        next(i_signal,i_len, i_repeat);
        m_tickNr = m_len; // enforce reload
    }

    void reset() {
        m_signal = m_next_signal = 0;
        m_signal_pending = m_signal_repeat = false;
        m_tickNr = m_len = 0;
    }

    void setIdle( unsigned i_signal, unsigned i_len=DEFAULT_SIGLEN){
        m_idle_signal = i_signal;
        m_idle_len = i_len;
    }

    bool tick();
};

#define FLEXIBLE_CODE

#ifdef FLEXIBLE_CODE
/*
 * flexible code allows for arbitrary patterns
 */
struct  MorseSymbol {
    unsigned len:6;
    unsigned pattern:28;
    MorseSymbol( unsigned i_len=0, unsigned i_sym=0):len(i_len),pattern(i_sym){}
};
#else
/* traditional morse code pattern builing only
 * a symbol is coded in one byte  from LSB to MSB
 * a pattern is represented by
 *   bit == 0  => dot
 *   bit == 1  => dash
 *   symbol end mark is the most significant 1-bit
 *   this end marker is not part of the symbol pattern
 *   to read the pattern MSB is read and then the pattern is right shifted.
 *   when pattern value EQUALS 1 => pattern end is reached
 */
typedef byte  MorseSymbol;

#endif



/**
 * MorseCoder is a specialized BlinkSignal
 *  a dot has length of one tick
 *  a dash is three ticks
 *  a pause within a symbol is one tick
 *  a pause between symbols is three ticks
 */
class MorseCoder {
    unsigned long m_lastTick;
    unsigned      m_ticklen;
    MorseSymbol   m_symbol;
    unsigned      m_subtick;  ///< ticks for this tickbit left
    bool 		  m_tickbit;  ///< the tickbit
    const char*   m_msg;      ///< the message to send as text
    unsigned      m_len;
    const char*   m_next_msg;
    bool          m_repeat;
    unsigned      m_rd;
    static MorseSymbol symTableAlpha[];
    static MorseSymbol symTableNum[];

public:
    static MorseSymbol encode( char c );

    MorseCoder(unsigned i_ticklen= 250):m_lastTick(0),m_ticklen( i_ticklen), m_symbol({0,0}),m_msg(0),m_len(0), m_next_msg(0), m_repeat(false),m_rd(0){}


    void next( const char* i_msg, bool i_repeat = false){
        m_repeat = i_repeat;
        m_next_msg = i_msg;
        if(!m_msg) {
            m_rd=m_len;
            m_lastTick = millis();
        }
    }

    void set( const char* i_msg, bool i_repeat = false){
        next(i_msg, i_repeat);
        m_rd = m_len;
    }

    void reset() {
        m_rd = m_len = 0;
        m_msg = m_next_msg = 0;
    }

    bool is_active() {
        //		Serial.printf(" is active: %s\n", m_msg ? "TRUE" : "false");
        return (m_msg  || m_next_msg);
    }

    bool is_idle() {
        return (m_msg ==0) && (m_next_msg == 0);
    }
    bool tick();

    static const MorseSymbol point;
    static const MorseSymbol dash;
    static const MorseSymbol space;
};



//--------------------------------------------
//--- EEPROM Helper --------------------------
//--------------------------------------------

#define EEPROM_SIZE 512
#define OPEN_EEPROM() eeprom_wp = 0; EEPROM.begin(EEPROM_SIZE)
#define SEEK_SET_EEPROM( pos ) eeprom_wp=(pos)
#define SEEK_REL_EEPROM( pos ) eeprom_wp+=(pos)
#define GET_EEPROM( var )  EEPROM.get(eeprom_wp, var); eeprom_wp+=sizeof(var); DEBUG_PRINT("[PERS] GET <" #var "> wp:%u\n", eeprom_wp )
#define PUT_EEPROM( var )  EEPROM.put(eeprom_wp, var); eeprom_wp+=sizeof(var); DEBUG_PRINT("[PERS] PUT <" #var "> wp:%u\n", eeprom_wp )
#define COMMIT_EEPROM()    EEPROM.commit()
#define CLOSE_EEPROM()     EEPROM.begin(EEPROM_SIZE)


#endif
