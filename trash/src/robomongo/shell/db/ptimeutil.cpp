/*
    wdb - weather and water data storage

    Copyright (C) 2007 met.no

    Contact information:
    Norwegian Meteorological Institute
    Box 43 Blindern
    0313 OSLO
    NORWAY
    E-mail: wdb@met.no

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
    MA  02110-1301, USA
*/
#include "ptimeutil.h"
#include <ctype.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
//#include <trimstr.h>
#include <sstream>

namespace
{
    int getInt( const std::string &timebuf, std::string::size_type &index, int numberOfChar,bool &isSuccessfull )
    {
        int nextN=0;
        char buf[10]={0};
        isSuccessfull = numberOfChar < 9;
        assert(isSuccessfull);		
        while( nextN < numberOfChar && isSuccessfull) {
            isSuccessfull = index < timebuf.length() && isdigit( timebuf[index] );     
            assert(isSuccessfull);			
            buf[nextN]=timebuf[index];
            nextN++;
            index++;
        }
        return atoi( buf );
    }
}
namespace miutil 
{
    const long long minDate = -9218988800000; // "1677-11-10T17:46:40.001Z"
    const long long maxDate = 9218988800000;  // "2262-02-20T06:13:19.999Z"

    std::string rfc1123date( const boost::posix_time::ptime &pt )
    {
        const char *day = NULL;
        const char *mon = NULL;

        if( pt.is_special() )
            return std::string();

        boost::gregorian::date d( pt.date() );
        boost::posix_time::time_duration t( pt.time_of_day() );

        switch( d.day_of_week() ) {
            using namespace boost::date_time;
        case Sunday:    day="Sun"; break;
        case Monday:    day="Mon"; break;
        case Tuesday:   day="Tue"; break;
        case Wednesday: day="Wed"; break;
        case Thursday:  day="Thu"; break;
        case Friday:    day="Fri"; break;
        case Saturday:  day="Sat"; break;
        }

        switch( d.month() ){
            using namespace boost::date_time;
        case Jan: mon="Jan"; break;
        case Feb: mon="Feb"; break;
        case Mar: mon="Mar"; break;
        case Apr: mon="Apr"; break;
        case May: mon="May"; break;
        case Jun: mon="Jun"; break;
        case Jul: mon="Jul"; break;
        case Aug: mon="Aug"; break;
        case Sep: mon="Sep"; break;
        case Oct: mon="Oct"; break;
        case Nov: mon="Nov"; break;
        case Dec: mon="Dec"; break;
        }

        if( !day || !mon )
            return std::string();

        unsigned short year = d.year();
        char buf[64] = {0};

        sprintf( buf, "%s, %02d %s %04d %02d:%02d:%02d GMT", day, d.day().as_number(), mon, year,t.hours(), t.minutes(), t.seconds() );

        return buf;
    }

    boost::posix_time::ptime rfc1123date( const std::string &rfc1123 )
    {
        return rfc1123date( rfc1123.c_str() );
    }

    boost::posix_time::ptime rfc1123date( const char *rfc1123 )
    {
        char gmt[4];
        char sday[4];
        char smon[4];
        int  day, mon, year, h, m, s;
        int  weekDay;

        if( !rfc1123 )
            return boost::posix_time::ptime( boost::date_time::not_a_date_time );

        if ( sscanf( rfc1123,"%3s, %02d %3s %04d %02d:%02d:%02d %3s", sday, &day, smon, &year,
            &h, &m, &s, gmt) !=8 ) 
            return boost::posix_time::ptime( boost::date_time::not_a_date_time );

        if( strcmp(gmt, "GMT")!=0 )
            return boost::posix_time::ptime( boost::date_time::not_a_date_time );

        if ( strcmp(smon, "Jan")==0 )
            mon=1;
        else if ( strcmp(smon, "Feb")==0 )
            mon=2;
        else if ( strcmp(smon, "Mar")==0 )
            mon=3;
        else if ( strcmp(smon, "Apr")==0 )
            mon=4;
        else if ( strcmp(smon, "May")==0 )
            mon=5;
        else if ( strcmp(smon, "Jun")==0 )
            mon=6;
        else if ( strcmp(smon, "Jul")==0 )
            mon=7;
        else if ( strcmp(smon, "Aug")==0 )
            mon=8;
        else if ( strcmp(smon, "Sep")==0 )
            mon=9;
        else if ( strcmp(smon, "Oct")==0 )
            mon=10;
        else if ( strcmp(smon, "Nov")==0 )
            mon=11;
        else if ( strcmp(smon, "Dec")==0 )
            mon=12;
        else
            return boost::posix_time::ptime( boost::date_time::not_a_date_time );
      
        //The day chack is a consistent check that could be dropped.
        if ( strcmp(sday, "Sun")==0 )
            weekDay=0;
        else if ( strcmp(sday, "Mon")==0 )
            weekDay=1;
        else if ( strcmp(sday, "Tue")==0 )
            weekDay=2;
        else if ( strcmp(sday, "Wed")==0 )
            weekDay=3;
        else if ( strcmp(sday, "Thu")==0 )
            weekDay=4;
        else if ( strcmp(sday, "Fri")==0 )
            weekDay=5;
        else if ( strcmp(sday, "Sat")==0 )
            weekDay=6;
        else
            return boost::posix_time::ptime( boost::date_time::not_a_date_time );

        if( day<0 || day>31 || h<0 || h>23 || m<0 || m>59 || s<0 || s>60 )
            return boost::posix_time::ptime( boost::date_time::not_a_date_time );   

        boost::posix_time::ptime date( boost::gregorian::date(year, mon, day) , 
            boost::posix_time::time_duration( h, m, s )  );
   
        //This is a consistent check. We could skip this test.
        if( weekDay != date.date().day_of_week() )
            return boost::posix_time::ptime( boost::date_time::not_a_date_time );

        return date;
    }

    std::string isotimeString(const boost::posix_time::ptime &pt, bool useTseparator, bool isLocalFormat)
    { 
        if( pt.is_special() )
            return "";

        char buf[64]={0};
        char sep=' '; 

        if( useTseparator )
            sep = 'T';

        boost::gregorian::date d( pt.date() );
        boost::posix_time::time_duration t( pt.time_of_day() );

        if( !isLocalFormat ){
            sprintf( buf, "%04d-%02d-%02d%c%02d:%02d:%02d.%03dZ", 
                static_cast<int>(d.year()), d.month().as_number(), d.day().as_number(), sep,
                t.hours(), t.minutes(), t.seconds(),(static_cast<int64_t>(t.total_milliseconds()))%1000 );
        }
        else{
            boost::posix_time::ptime timeP(d,t);       
            time_t rawtime;
            time ( &rawtime );
            struct tm *ptm = std::gmtime ( &rawtime );
            int utcH = ptm->tm_hour;
            int utcM = ptm->tm_min;
            struct tm *timeinfo = std::localtime (&rawtime);
            int diffH = timeinfo->tm_hour - utcH;
            int diffM = timeinfo->tm_min - utcM;
            boost::posix_time::time_duration diffT = boost::posix_time::time_duration(diffH, diffM, 0);
            timeP += diffT;

            d = timeP.date();
            t = timeP.time_of_day();

            char utc_buff[8]={0};
            sprintf(utc_buff,diffT.hours()>0?"+%02d:%02d":"%03d:%02d",diffT.hours(),abs(diffM));
            sprintf( buf, "%04d-%02d-%02d%c%02d:%02d:%02d.%03d", 
                static_cast<int>(d.year()), d.month().as_number(), d.day().as_number(), sep,
                t.hours(), t.minutes(), t.seconds(),(static_cast<int64_t>(t.total_milliseconds()))%1000);
            strcat(buf,utc_buff);
        }

        return buf;   
    }

    boost::posix_time::ptime ptimeFromIsoString( const std::string &isoTime)
    {
        bool isSuccessfull=false;
        return ptimeFromIsoString(isoTime,isSuccessfull);
    }

    boost::posix_time::ptime ptimeFromIsoString( const std::string &isoTime, bool &isSuccessfull)
    {
        struct DEF {
            int number;
            unsigned char numberOfChars;
            const char *nextSep;
        };
	
        DEF def[] = {
                        {0, 4, "-"},  //year
                        {0, 2, "-"},  //Month
                        {0, 2, "T "},//day
                        {0, 2, ":"}, //hour
                        {0, 2, ":"}, //minute
                        {0, 2, ".Z"}, //second
                        {0, 3, "+-Z"}, //msecond
                        {0, 0, 0}    //terminator
                    };

        isSuccessfull = true;

        if( isoTime == "infinity" )
            return boost::posix_time::ptime( boost::posix_time::pos_infin );
        else if( isoTime == "-infinity" )
            return boost::posix_time::ptime( boost::posix_time::neg_infin );
        else if( isoTime == "epoch" )
            return boost::posix_time::ptime( boost::gregorian::date( 1970, 1, 1 ),
            boost::posix_time::time_duration( 0, 0, 0) );
        else if( isoTime == "now" )
            return boost::posix_time::second_clock::universal_time();
        else if( isoTime == "today" ) {
            boost::posix_time::ptime now( boost::posix_time::second_clock::universal_time() );
            return boost::posix_time::ptime( now.date(), boost::posix_time::time_duration( 0, 0, 0) );
        } else if( isoTime == "tomorrow" ) {
            boost::posix_time::ptime now( boost::posix_time::second_clock::universal_time() );
            now += boost::posix_time::hours(24);
            return boost::posix_time::ptime( now.date(), boost::posix_time::time_duration( 0, 0, 0) );
        } else if( isoTime == "yesterday" ) {
            boost::posix_time::ptime now( boost::posix_time::second_clock::universal_time() );
            now -= boost::posix_time::hours(24);
            return boost::posix_time::ptime( now.date(), boost::posix_time::time_duration( 0, 0, 0) );
        }

        std::string::size_type iIsoTime = isoTime.find_first_not_of( " ", 0 ); //Skip whitespace in front
        isSuccessfull = iIsoTime != std::string::npos;
        assert( isSuccessfull );
	
        if( ! isdigit( (int)isoTime[iIsoTime] ) ) {
        //We try to decode it as an rfc1123date.
            boost::posix_time::ptime pt = rfc1123date( isoTime.c_str() );
            isSuccessfull = !pt.is_special();
            assert( isSuccessfull );	
            return pt;
        }
	
        //Decode the YYYY-MM-DDThh:mm:ss part
        //Remeber -, T and : is optional.
        for( unsigned short defIndex=0; def[defIndex].nextSep && iIsoTime < isoTime.length() && isSuccessfull; ++defIndex ) {
            def[defIndex].number = getInt( isoTime, iIsoTime, def[defIndex].numberOfChars, isSuccessfull);	
            iIsoTime = isoTime.find_first_of( def[defIndex].nextSep, iIsoTime );
            if(defIndex!=6)
                ++iIsoTime;
        }

        if(!isSuccessfull){
            return boost::posix_time::ptime();
        } 

        int hourOffset=0;
        int minuteOffset=0;
        //Decode the UTC offset part of
        //YYYY-MM-DDThh:mm:ssSHHMM part
        //The UTC offset part is SHHMM
        //Where S is the sign. + or -.
        //      HH the hour offset, mandatory if we have an UTC offset part.
        //      MM the minute offset, otional.
        if( iIsoTime < isoTime.length() ) {
            //We have a possible UTC offset.
            //The format is SHHMM, where S is the sign.
            char ch = isoTime[iIsoTime];
		
            if( ch == '+' || ch == '-' || isdigit( ch ) ) {
                int sign=1;

                if( ! isdigit( ch ) ) {
                    if( ch=='-' )
                        sign=-1;
                    isSuccessfull = isdigit( isoTime[++iIsoTime] )!=0;
                    assert( isSuccessfull );
                }
                hourOffset = getInt( isoTime, iIsoTime, 2, isSuccessfull);
                hourOffset*=sign;

                if( iIsoTime < isoTime.length() && isdigit( isoTime[++iIsoTime] ) ) {
                    minuteOffset = getInt( isoTime, iIsoTime, 2, isSuccessfull);
                }
            }
        }

        try{
            boost::gregorian::date date(def[0].number, def[1].number, def[2].number);
            boost::posix_time::time_duration td(def[3].number, def[4].number, def[5].number,def[6].number*1000);
            boost::posix_time::ptime pt(date, td);
            td = boost::posix_time::time_duration(hourOffset, minuteOffset, 0);
            return pt - td;
        }
        catch(const std::out_of_range &ex)
        {
            isSuccessfull = false;
            return boost::posix_time::ptime();
        }
    }

}