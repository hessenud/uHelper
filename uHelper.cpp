#include <uHelper.h>


//-------------------------------------------------------------------
//-- TimeClk -------------------------------------------------
//-------------------------------------------------------------------

unsigned long TimeClk::daytime2unixtime(unsigned long i_daytime, unsigned long _now) {
  unsigned long dayoffset = _now - (_now%(1 Day));
  return dayoffset +  i_daytime;
}

/*
 * © Francesco Potortì 2013 - GPLv3 - Revision: 1.13
 *
 * Send an NTP packet and wait for the response, return the Unix time
 *
 * To lower the memory footprint, no buffers are allocated for sending
 * and receiving the NTP packets.  Four bytes of memory are allocated
 * for transmision, the rest is random garbage collected from the data
 * memory segment, and the received packet is read one byte at a time.
 * The Unix time is returned, that is, seconds from 1970-01-01T00:00.
 */
unsigned long TimeClk::ntpUnixTime()
{
	static int udpInited = m_udp.begin(123); // open socket on arbitrary port

	//const char timeServer[] = "pool.ntp.org";  // NTP server
	const char m_ntpServer[] = "fritz.box";  // NTP server

	// Only the first four bytes of an outgoing NTP packet need to be set
	// appropriately, the rest can be whatever.
	const long ntpFirstFourBytes = 0xEC0600E3; // NTP request header

	// Fail if WiFiUdp.begin() could not init a socket
	if (! udpInited)
		return 0;

	// Clear received data from possible stray received packets
	m_udp.flush();

	// Send an NTP request
	if (! (m_udp.beginPacket(m_ntpServer, 123) // 123 is the NTP port
			&& m_udp.write((byte *)&ntpFirstFourBytes, 48) == 48
			&& m_udp.endPacket()))
		return 0;       // sending request failed

	// Wait for response; check every pollIntv ms up to maxPoll times
	const int pollIntv = 150;   // poll every this many ms
	const byte maxPoll = 15;    // poll up to this many times
	int pktLen;       // received packet length
	for (byte i=0; i<maxPoll; i++) {
		if ((pktLen = m_udp.parsePacket()) == 48)
			break;
		delay(pollIntv);
	}
	if (pktLen != 48)
		return 0;       // no correct packet received

	// Read and discard the first useless bytes
	// Set useless to 32 for speed; set to 40 for accuracy.
	const byte useless = 40;
	for (byte i = 0; i < useless; ++i)
		m_udp.read();

	// Read the integer part of sending time
	unsigned long time = m_udp.read();  // NTP time
	for (byte i = 1; i < 4; i++)
		time = time << 8 | m_udp.read();

	// Round to the nearest second if we want accuracy
	// The fractionary part is the next byte divided by 256: if it is
	// greater than 500ms we round to the next second; we also account
	// for an assumed network delay of 50ms, and (0.5-0.05)*256=115;
	// additionally, we account for how much we delayed reading the packet
	// since its arrival, which we assume on average to be pollIntv/2.
	time += (m_udp.read() > 115 - pollIntv/8);

	// Discard the rest of the packet
	m_udp.flush();

	return time - 2208988800ul;   // convert NTP time to Unix time
}

unsigned long TimeClk::read()
	{
		unsigned char runs=0;
		re_run:

		if ( runs) Serial.printf_P(PSTR("read time %u\n"), runs );
		if (!m_ntpTime )
		{
			m_ntpTime = ntpUnixTime();
			m_ltime_ms = millis();
			unsigned long newLocalTime = utc2local( m_ntpTime );
			if ( newLocalTime > m_localTime ) ++m_tickLen;
			else  if ( newLocalTime < m_localTime ) --m_tickLen;
			Serial.printf_P(PSTR("new NTP Time %s vs %s new ticklen: %d\n")
					, String(TimeClk::getTimeString(newLocalTime)).c_str(), TimeClk::getTimeString(m_localTime), m_tickLen);
			m_localTime = newLocalTime;
		}
		if ( m_ntpTime ) {
			// update time locally
			unsigned now = millis();
			int dt = now - m_ltime_ms;
			while ( dt >= 1000 ){
				++m_localTime;
				m_ltime_ms += m_tickLen;
				dt -= m_tickLen;
			}
			if ( 0 == (m_localTime%600) && !runs ) {
				m_ntpTime = 0;
				++runs;
				Serial.printf_P(PSTR("actualize ntp time\n"));
				goto re_run;
			}
		}

		//Serial.printf_P(PSTR("read time %s\n"), TimeClk::getTimeString(m_localTime) );
		return m_localTime + ( m_dst ? 1 : 0);
	}


const char* TimeClk::getTimeString( unsigned long theTime )
{
	static char timestr[9];
	// print the hour, minute and second:
	sprintf_P( timestr, PSTR("%2lu:%02lu:%02lu"), ((theTime  % 86400L) / 3600), ((theTime  % 3600) / 60), theTime % 60);

	return timestr;
}

const char* TimeClk::getTimeStringS( unsigned long theTime )
{
	static char timestr[9];
	// print the hour and minute:
	sprintf_P( timestr, PSTR("%2lu:%02lu"), ((theTime  % 86400L) / 3600), ((theTime  % 3600) / 60) );

	return timestr;
}

const char* TimeClk::getTimeStringDays( unsigned long theTime )
{
	static char timestr[9];
	// print the hour and minute:
	sprintf_P( timestr, PSTR("%lu:>"), (theTime  /86400L) );

	return timestr;
}



//-------------------------------------------------------------------
//-- IO - Key input -------------------------------------------------
//-------------------------------------------------------------------

int PushButton::getEvent(){

	static const char* evt2str[] ={
			"IDLE",
			"UP",
			"PRESSED",  // DN - UP
			"DN",
			"DN_LONG1",
			"DN_LONG2",
			"DBLCLICK",
			"TRIPLECLICK",
			"QUADCLICK",
			"---"
	};

	int event = IDLE; //
	int input = read();


	if ( input == LOW ){
		unsigned long dT = millis() - m_dnTime;
		switch( m_btn_state ) {
		case sUP:
		case sUP_1:
			if ( dT > m_multiclick_threshold ) {
				m_clicks = 0;
			}
			++m_clicks;
			m_btn_state = sDN;
			m_dnTime = millis();
			event = DN;
			break;
		case sDN:
			if ( dT > m_long1_threshold ) {
				event = DN_LONG1;
				m_btn_state = sDN_LONG1;
			}
			break;
		case sDN_LONG1:
			if ( dT > m_long2_threshold ) {
				event = DN_LONG2;
				m_btn_state = sDN_LONG2;
			}
			break;
		}
	} else {
		// Key released
		unsigned long dT = millis() - m_upTime;

		switch( m_btn_state ) {
		case sUP:
			break;
		case sUP_1:
			if ( dT > m_multiclick_threshold ) {
				event = CLICK;
				m_btn_state = sUP;
			} else {
			}
			break;
		case sDN:
			m_upTime = millis();
			switch (m_clicks) {
			//case 1: event =  PRESSED; 		break;
			case 2: event =  DBLCLICK;		break;
			case 3: event =  TRIPLECLICK;	break;
			case 4: event =  QUADCLICK;		break;
			default:
				{
					event = IDLE;
					m_btn_state = sUP_1;
					return event;
				}
			}
			break;
		case sDN_LONG1:
			event = DN_LONG1;
			break;
		case sDN_LONG2:
			event = DN_LONG2;
			break;

		default: ;
		}
		if (event) m_btn_state = sUP;

	}

	if ( event ) Serial.printf(" PushButtonEvent: %s\n", evt2str[event]);
	return event;

}


bool BlinkSignal::tick(){
	unsigned long now = millis();
	if ( now - m_lastTick > m_ticklen ) {
		m_lastTick += m_ticklen; // incrementally compensate latencies
		m_signal >>= 1; // shift out first bit
		if ( ++m_tickNr >= m_len )
		{	// reload
			m_tickNr = 0;
			if ( m_signal_pending ) {
				m_idle = false;
				m_signal = m_next_signal;
				m_len = m_next_len;
				m_signal_pending = m_signal_repeat;
			} else {
				m_idle = true;
				m_signal = m_idle_signal;
				m_len    = m_idle_len;
			}
		}
	}
	return (m_signal&1) != 0;
}




const MorseSymbol MorseCoder::point( 1, B1 );
const MorseSymbol MorseCoder::dash( 3, 0x7 );
const MorseSymbol MorseCoder::space( 1, 0 );
const MorseSymbol MorseCoder::long_space( 6, 0 );

MorseSymbol MorseCoder::symTableAlpha[]  = {
    //   read symbols-pattern from "right to left"
    {5 , 0b010111   },        //"-.",     // a
    {9 , 0b101010111 },       // "-...",   // b
    {11, 0b10111010111 },     // "-.-.",   // c
    {7 , 0b1010111 },         // "-..",    // d
    {1 , 0b1}, // ".",      // e
    {9 , 0b101110101}, // "..-.",   // f
    {9 , 0b101110111}, // "--.",    // g
    {7 , 001010101}, // "....",   // h
    {3 , 0b101}, // "..",     // i
    {9 , 0b101110111}, // ".---",   // j
    {9 , 0b111010111}, // "-.-",    // k
    {9 , 0b101011101}, // ".-..",   // l
    {8 , 0b11110111}, // "--",     // m
    {5 , 0b10111}, // "-.",     // n
    {11, 0b11101110111}, // "---",    // o
    {11, 0b10111011101}, // ".--.",   // p
    {13, 0b1110101110111}, // "--.-",   // q
    {7 , 0b1011101}, // ".-.",    // r
    {5 , 0b10101}, // "...",    // s
    {3 , 0b111}, // "-",      // t
    {7 , 0b1110101}, // "..-",    // u
    {9 , 0b111010101}, // "...-",   // v
    {9 , 0b101110111}, // ".--",    // w
    {11, 0b11101010111}, // "-..-",   // x
    {13, 0b1110111010111}, // "-.--",   // y
    {11, 0b10101110111} // "--..",   // z
};

MorseSymbol MorseCoder::symTableNum[]  = {
    //   read symbols-pattern from "right to left"
    // 0x30
    {19, 0b1110111011101110111 }, // "-----"   // 0
    {17, 0b11101110111011101}, // ".----",  // 1
    {16, 0b111011101110101}, // "..---",  // 2
    {13, 0b1110111010101}, // "...--",  // 3
    {11, 0b11101010101}, // "....-",  // 4
    {9 , 0b101010101}, // ".....",  // 5
    {11, 0b10101010111}, // "-....",  // 6
    {13, 0b1010101110111}, // "--...",  // 7
    {15, 0b101011101110111}, // "---..",  // 8
    {17, 0b10111011101110111} // "----.",  // 9
  };

MorseSymbol MorseCoder::encode( char c )
{
  MorseSymbol sym = {0,0};
  unsigned idx=0; // = unsigned(c - 'a');
  c = tolower( c );
  if ( c >= '0' && c <= '9') {
    idx = unsigned(c - '0');
    sym = symTableNum[idx];
  } else if ( c >= 'a' && c <= 'z') {
    idx = unsigned(c - 'a');
    sym = symTableAlpha[idx];
  } else {
    // SPEACIAL
    switch(c) {
      case '?': sym = MorseSymbol(15 , 0b101011101110101  );  break;  /*· · − − · ·*/
      default: //Serial.printf("symbol not found: %c\n", c);
    	  ;
    }
  }
  //Serial.printf("encode %c -> idx:%u  (%u, 0x%x)\n", c, idx,sym.len, sym.pattern );
  return sym;

}

bool MorseCoder::tick(){
  unsigned long now = millis();
  bool tickbit = ( m_symbol.pattern&1);
  if ( now - m_lastTick > m_ticklen ) {
//	if(m_msg)  Serial.printf(" Morse TICK msg: %s len:%u / %u    ACTIVE\n",&m_msg[m_rd], m_len, m_rd);

    m_lastTick += m_ticklen; // incrementally compensate latencies

    if ( m_symbol.len )
    { // symbol not empty
      m_symbol.pattern >>= 1; // shift out first bit
      --m_symbol.len;       // decrementing after shift effectively adds 1 tich/dot pause to every symbol
    } else {
      // reload from msg
      if ( m_rd < m_len ) {
        switch( m_msg[m_rd] )
        {
          case '-': m_symbol = dash;  break;  // raw morse is still possible
          case '.': m_symbol = point; break;  // so one tick = intra symbol pause
          case ' ': m_symbol = space; break;  // is always added
          default:  m_symbol = encode( m_msg[m_rd] );  // complex symbols bring their own pause (3ticks)
                    m_symbol.len += 2; // inter symbol space is one dash 3 Ticks
                                       // add 2 Ticks here, one tick is always added
        }
        // add 1 tick/dotlen pause to every Symbol
        // by  decrementing len AFTER shifting out pattern
        ++m_rd;
      } else {
        // reload next msg if pending
        if ( m_next_msg ) {
          m_msg = m_next_msg;
          m_len = strlen(m_msg);
     //     Serial.printf(" nxt msg: %s len:%u    %s\n",m_msg,m_len, is_active() ? "ACTIVE":"");
          m_next_msg = 0; // clear nxt msg buffer
          tickbit = false;
        } else {
          if ( m_repeat )
          { // repetition, so leave the msg and only reset rd pointer
          } else {
       // 	  Serial.println("tick IDLE");
            m_msg = 0;  // no repetition so delete msg
            m_len = 0;
            tickbit = false;
          }
        }
        m_rd = 0; // reset rd pointer
      }
    }
  }
  return tickbit;
}




bool uTimer::check(unsigned long _time)
{
  //Serial.printf("check %p:  time %s(%lu)  vs: %s(%lu) %s\n", this, String(TimeClk::getTimeStringS(m_swTime)).c_str(),m_swTime, String(TimeClk::getTimeStringS(_time)).c_str(),_time, m_hot ? "ARMED" : "IDLE" );
  if(m_swTime >= _time)
  {
    if ( m_hot  ) // hot an time reached
    {
      trigger();
      return true;
    }
  } else {
    if (m_interval) rearm( 0 );
  }
  return false;
}


void uTimer::append_to(uTimer* i_list)
{
  uTimer* wp= i_list;
  if (wp){
    while(  wp->next()) wp = wp->next();
    wp->m_next = this;
  }
}

void uTimer::append(uTimer* i_tmr)
{
  uTimer* wp= this;
  while(  wp->m_next)  wp = wp->next();
  wp->m_next = i_tmr;
}


void uTimer::trigger()
{
  if (m_channel) *m_channel = m_switchmode;
  if (m_channel_cb) m_channel_cb( m_switchmode );
  m_hot = false;
}
