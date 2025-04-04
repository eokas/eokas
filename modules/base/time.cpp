
#include "./time.h"
#include "./string.h"
#include <limits>

namespace eokas {
    /*
    ============================================================================================
    ==== TimeSpan
    ============================================================================================
    */
    const i64_t TimeSpan::US_COUNT_PER_MS = 1000;
    const i64_t TimeSpan::US_COUNT_PER_SECOND = 1000000;
    const i64_t TimeSpan::US_COUNT_PER_MINUTE = 60000000;
    const i64_t TimeSpan::US_COUNT_PER_HOUR = 3600000000;
    const i64_t TimeSpan::US_COUNT_PER_DAY = 86400000000;
    
    TimeSpan TimeSpan::maxValue() {
        return TimeSpan(LLONG_MAX);
    }
    
    TimeSpan TimeSpan::minValue() {
        return TimeSpan(LLONG_MIN);
    }
    
    TimeSpan::TimeSpan(i64_t usecs)
        : mSpan(usecs) {
    }
    
    TimeSpan::TimeSpan(i32_t days, i32_t hours, i32_t mins, i32_t secs, i32_t msecs, i32_t usecs)
        : mSpan(US_COUNT_PER_DAY * days + US_COUNT_PER_HOUR * hours + US_COUNT_PER_MINUTE * mins + US_COUNT_PER_SECOND * secs + US_COUNT_PER_MS * msecs + usecs) {
    }
    
    TimeSpan::TimeSpan(const TimeSpan& other)
        : mSpan(other.mSpan) {
    }
    
    TimeSpan::~TimeSpan() {
    }
    
    TimeSpan& TimeSpan::operator=(const TimeSpan& other) {
        mSpan = other.mSpan;
        return *this;
    }
    
    TimeSpan TimeSpan::operator+(const TimeSpan& other) {
        return TimeSpan(mSpan + other.mSpan);
    }
    
    TimeSpan TimeSpan::operator-(const TimeSpan& other) {
        return TimeSpan(mSpan - other.mSpan);
    }
    
    TimeSpan& TimeSpan::operator+=(const TimeSpan& other) {
        mSpan += other.mSpan;
        return *this;
    }
    
    TimeSpan& TimeSpan::operator-=(const TimeSpan& other) {
        mSpan -= other.mSpan;
        return *this;
    }
    
    bool TimeSpan::operator>(const TimeSpan& other) {
        return mSpan > other.mSpan;
    }
    
    bool TimeSpan::operator<(const TimeSpan& other) {
        return mSpan < other.mSpan;
    }
    
    bool TimeSpan::operator>=(const TimeSpan& other) {
        return mSpan >= other.mSpan;
    }
    
    bool TimeSpan::operator<=(const TimeSpan& other) {
        return mSpan <= other.mSpan;
    }
    
    bool TimeSpan::operator==(const TimeSpan& other) {
        return mSpan == other.mSpan;
    }
    
    bool TimeSpan::operator!=(const TimeSpan& other) {
        return mSpan != other.mSpan;
    }
    
    i64_t TimeSpan::dayPart() const {
        return mSpan / US_COUNT_PER_DAY;
    }
    
    i64_t TimeSpan::hourPart() const {
        return mSpan % US_COUNT_PER_DAY / US_COUNT_PER_HOUR;
    }
    
    i64_t TimeSpan::minutePart() const {
        return mSpan % US_COUNT_PER_DAY % US_COUNT_PER_HOUR / US_COUNT_PER_MINUTE;
    }
    
    i64_t TimeSpan::secondPart() const {
        return mSpan % US_COUNT_PER_DAY % US_COUNT_PER_HOUR % US_COUNT_PER_MINUTE / US_COUNT_PER_SECOND;
    }
    
    i64_t TimeSpan::millisecondPart() const {
        return mSpan % US_COUNT_PER_DAY % US_COUNT_PER_HOUR % US_COUNT_PER_MINUTE % US_COUNT_PER_SECOND / US_COUNT_PER_MS;
    }
    
    i64_t TimeSpan::microsecondPart() const {
        return mSpan % US_COUNT_PER_MS;
    }
    
    i64_t TimeSpan::totalDays() const {
        i64_t result = mSpan / US_COUNT_PER_DAY;
        if (mSpan % US_COUNT_PER_DAY > 0) {
            result += 1;
        }
        return result;
    }
    
    i64_t TimeSpan::totalHours() const {
        i64_t result = mSpan / US_COUNT_PER_HOUR;
        if (mSpan % US_COUNT_PER_HOUR > 0) {
            result += 1;
        }
        return result;
    }
    
    i64_t TimeSpan::totalMinutes() const {
        i64_t result = mSpan / US_COUNT_PER_MINUTE;
        if (mSpan % US_COUNT_PER_MINUTE > 0) {
            result += 1;
        }
        return result;
    }
    
    i64_t TimeSpan::totalSeconds() const {
        i64_t result = mSpan / US_COUNT_PER_SECOND;
        if (mSpan % US_COUNT_PER_SECOND > 0) {
            result += 1;
        }
        return result;
    }
    
    i64_t TimeSpan::totalMilliseconds() const {
        i64_t result = mSpan / US_COUNT_PER_MS;
        if (mSpan % US_COUNT_PER_MS > 0) {
            result += 1;
        }
        return result;
    }
    
    f64_t TimeSpan::exactDays() const {
        f64_t result = mSpan / (US_COUNT_PER_DAY + 0.0);
        return result;
    }
    
    f64_t TimeSpan::exactHours() const {
        f64_t result = mSpan / (US_COUNT_PER_HOUR + 0.0);
        return result;
    }
    
    f64_t TimeSpan::exactMinutes() const {
        f64_t result = mSpan / (US_COUNT_PER_MINUTE + 0.0);
        return result;
    }
    
    f64_t TimeSpan::exactSeconds() const {
        f64_t result = mSpan / (US_COUNT_PER_SECOND + 0.0);
        return result;
    }
    
    f64_t TimeSpan::exactMilliseconds() const {
        f64_t result = mSpan / (US_COUNT_PER_MS + 0.0);
        return result;
    }
    
    /*
    ============================================================================================
    ==== TimePoint
    ============================================================================================
    */
#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
#define _mktime_utc _mkgmtime
#else
#define _mktime_utc timegm
#endif
    
    TimePoint::TimePoint()
        : mTimeStamp(time(nullptr)) {
        mTimeStruct = gmtime(&mTimeStamp);
    }
    
    TimePoint::TimePoint(time_t timeStamp)
        : mTimeStamp(timeStamp) {
        mTimeStruct = gmtime(&mTimeStamp);
    }
    
    TimePoint::TimePoint(int year, int month, int date, int hour, int minute, int sec) {
        tm utc;
        utc.tm_year = year - 1900;
        utc.tm_mon = month - 1;
        utc.tm_mday = date;
        utc.tm_hour = hour;
        utc.tm_min = minute;
        utc.tm_sec = sec;
        utc.tm_isdst = 0;
        
        mTimeStamp = _mktime_utc(&utc);
        mTimeStruct = gmtime(&mTimeStamp);
    }
    
    TimePoint::TimePoint(const TimePoint& other)
        : mTimeStamp(other.mTimeStamp) {
        mTimeStruct = gmtime(&mTimeStamp);
    }
    
    TimePoint::~TimePoint() {
    }
    
    TimePoint& TimePoint::operator=(const TimePoint& other) {
        mTimeStamp = other.mTimeStamp;
        mTimeStruct = gmtime(&mTimeStamp);
        return *this;
    }
    
    TimePoint TimePoint::operator+(const TimeSpan& span) {
        return TimePoint(time_t(mTimeStamp + span.totalSeconds()));
    }
    
    TimePoint TimePoint::operator-(const TimeSpan& span) {
        return TimePoint(time_t(mTimeStamp - span.totalSeconds()));
    }
    
    TimePoint& TimePoint::operator+=(const TimeSpan& span) {
        mTimeStamp += span.totalSeconds();
        mTimeStruct = gmtime(&mTimeStamp);
        return *this;
    }
    
    TimePoint& TimePoint::operator-=(const TimeSpan& span) {
        mTimeStamp -= span.totalSeconds();
        mTimeStruct = gmtime(&mTimeStamp);
        return *this;
    }
    
    TimeSpan TimePoint::operator-(const TimePoint& other) {
        return TimeSpan((mTimeStamp - other.mTimeStamp) * TimeSpan::US_COUNT_PER_SECOND);
    }
    
    bool TimePoint::operator==(const TimePoint& other) {
        return mTimeStamp == other.mTimeStamp;
    }
    
    bool TimePoint::operator!=(const TimePoint& other) {
        return mTimeStamp != other.mTimeStamp;
    }
    
    bool TimePoint::operator>(const TimePoint& other) {
        return mTimeStamp > other.mTimeStamp;
    }
    
    bool TimePoint::operator<(const TimePoint& other) {
        return mTimeStamp < other.mTimeStamp;
    }
    
    bool TimePoint::operator>=(const TimePoint& other) {
        return mTimeStamp >= other.mTimeStamp;
    }
    
    bool TimePoint::operator<=(const TimePoint& other) {
        return mTimeStamp <= other.mTimeStamp;
    }
    
    u64_t TimePoint::timestamp() const {
        return (u64_t) mTimeStamp;
    }
    
    i32_t TimePoint::year() const {
        return mTimeStruct->tm_year + 1900;
    }
    
    i32_t TimePoint::month() const {
        return mTimeStruct->tm_mon + 1;
    }
    
    i32_t TimePoint::date() const {
        return mTimeStruct->tm_mday;
    }
    
    i32_t TimePoint::hour() const {
        return mTimeStruct->tm_hour;
    }
    
    i32_t TimePoint::minute() const {
        return mTimeStruct->tm_min;
    }
    
    i32_t TimePoint::second() const {
        return mTimeStruct->tm_sec;
    }
    
    i32_t TimePoint::dayOfWeek() const {
        return mTimeStruct->tm_wday;
    }
    
    i32_t TimePoint::dayofYear() const {
        return mTimeStruct->tm_yday;
    }
    
    bool TimePoint::isLeapYear() const {
        i32_t year = this->year();
        return (year % 4 == 0) || (year % 400 == 0);
    }
    
    String TimePoint::toString(const String& fmt) {
        String result = fmt;
        
        result = result.replace("YYYY", String::format("%04d", this->year()));
        result = result.replace("YYY", String::format("%03d", this->year() % 1000));
        result = result.replace("YY", String::format("%02d", this->year() % 100));
        result = result.replace("Y", String::format("%d", this->year()));
        
        result = result.replace("MM", String::format("%02d", this->month()));
        result = result.replace("M", String::format("%d", this->month()));
        
        result = result.replace("DD", String::format("%02d", this->date()));
        result = result.replace("D", String::format("%d", this->date()));
        
        result = result.replace("hh", String::format("%02d", this->hour()));
        result = result.replace("h", String::format("%d", this->hour()));
        
        result = result.replace("mm", String::format("%02d", this->minute()));
        result = result.replace("m", String::format("%d", this->minute()));
        
        result = result.replace("ss", String::format("%02d", this->second()));
        result = result.replace("s", String::format("%d", this->second()));
        
        return result;
    }
    
}
