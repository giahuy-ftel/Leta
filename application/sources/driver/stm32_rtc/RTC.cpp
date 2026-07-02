#include "RTC.h"
#include "main.h"
#ifdef __cplusplus
extern "C"
{
#endif
const char* 	dayNames[7]   = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
const char* 	monthNames[12]   = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

#define RTC_LSE_TIMEOUT         0x400000UL
#define RTC_LSI_TIMEOUT         0x400000UL
#define RTC_LSE_PRESCALER       (32768UL - 1UL)
#define RTC_LSI_PRESCALER       (40000UL - 1UL)
#define SECONDS_PER_DAY         86400UL

static uint32_t RTC_Init(void)
{	 
	//For RTC
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
	PWR_BackupAccessCmd(ENABLE);

	RCC_LSEConfig(RCC_LSE_ON);
	uint32_t timeout = RTC_LSE_TIMEOUT;
	while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET && timeout-- > 0);

	if(RCC_GetFlagStatus(RCC_FLAG_LSERDY) != RESET)
	{
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
		RCC_RTCCLKCmd(ENABLE);
		PWR_BackupAccessCmd(DISABLE);
		return RTC_LSE_PRESCALER;
	}

	RCC_LSICmd(ENABLE);
	timeout = RTC_LSI_TIMEOUT;
	while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET && timeout-- > 0);

	if(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
	{
		PWR_BackupAccessCmd(DISABLE);
		return 0;
	}

	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
	RCC_RTCCLKCmd(ENABLE);
	PWR_BackupAccessCmd(DISABLE);
	return RTC_LSI_PRESCALER;
}

static bool isLeapYear(unsigned int year)
{
   // leap year if perfectly divisible by 400
   if (year % 400 == 0) return 1;
	
   // not a leap year if divisible by 100
   // but not divisible by 400
   else if (year % 100 == 0) return 0;

   // leap year if not divisible by 100
   // but divisible by 4
   else if (year % 4 == 0) return 1;
	
   // all other years are not leap years
	else return 0;
}

static uint16_t daysInYear(uint16_t year)
{
	return isLeapYear(year) ? 366 : 365;
}

static uint8_t daysInMonth(uint16_t year, uint8_t month)
{
	if(month == 2 && isLeapYear(year)) return 29;
	return day_of_month[month];
}

static uint32_t dateToDays(uint16_t year, uint8_t month, uint8_t day)
{
	uint32_t days = 0;

	for(uint16_t y = BASE_YEAR; y < year; y++)
	{
		days += daysInYear(y);
	}

	for(uint8_t m = BASE_MONTH; m < month; m++)
	{
		days += daysInMonth(year, m);
	}

	return days + (uint32_t)(day - 1);
}

static void normalizeDateTime(uint16_t *year, uint8_t *month, uint8_t *day, uint8_t *hour, uint8_t *minute, uint8_t *second)
{
	if(*year < BASE_YEAR) *year = BASE_YEAR;
	if(*month < 1) *month = 1;
	if(*month > 12) *month = 12;

	const uint8_t max_day = daysInMonth(*year, *month);
	if(*day < 1) *day = 1;
	if(*day > max_day) *day = max_day;

	if(*hour > 23) *hour = 23;
	if(*minute > 59) *minute = 59;
	if(*second > 59) *second = 59;
}

// This function will return week day number from 0 to 6
static uint8_t returnWeekday(uint16_t year, uint8_t month, uint8_t day)
{
    uint8_t wday = 0;
    wday = (day + ((153 * (month + 12 * ((14 - month) / 12) - 3) + 2) / 5)
               + (365 * (year + 4800 - ((14 - month) / 12)))
               + ((year + 4800 - ((14 - month) / 12)) / 4)
               - ((year + 4800 - ((14 - month) / 12)) / 100)
               + ((year + 4800 - ((14 - month) / 12)) / 400)
               - 32045)	% 7;
    return wday;
}

RTCSTM32::RTCSTM32(){}
RTCSTM32::~RTCSTM32(){}

void RTCSTM32::init(void)
{
	__disable_irq();
	uint32_t prescaler = RTC_Init();
	if(prescaler == 0)
	{
		__enable_irq();
		return;
	}

	PWR_BackupAccessCmd(ENABLE);
	RTC_WaitForSynchro();

	if(!IS_RTC_RUNNING()) //Init for a first time
	{				
		RTC_WaitForLastTask();			//This is quite important to wait before and after any write operation
		RTC_SetPrescaler(prescaler);
		RTC_WaitForLastTask();

		RTC_SetCounter(BASE_SECOND);
		RTC_WaitForLastTask();
		
		BKP->DR1 |= 0x8000; //assign first bit in DR1 in order not to init at the next time		
	}

	PWR_BackupAccessCmd(DISABLE);
	__enable_irq();
}
RTCDateTime RTCSTM32::getDateTime(void)
{
	RTC_WaitForLastTask();
	uint32_t counts  = RTC_GetCounter();
	uint16_t year;
	uint8_t month;
	uint32_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;

	day = (counts / SECONDS_PER_DAY);
	counts = counts - day * SECONDS_PER_DAY;
	hour = counts / 3600;			//Hour is done
	minute = (counts % 3600) / 60;	//Minute is done
	second = (counts % 3600) % 60;	//Second is done

	year = BASE_YEAR;
	while(day >= daysInYear(year))
	{
		day -= daysInYear(year);
		year++;			
	}
	//Year is done

	month = BASE_MONTH;
	while(month < 12 && day >= daysInMonth(year, month))
	{
		day -= daysInMonth(year, month);
		month++;
	}
	//Month is done
	//Day also
	day++;
	uint8_t dayOfWeek = returnWeekday(year, month, (uint8_t)day); //Day of week

	date_time.year = year;
	date_time.month = month;
	date_time.day = (uint8_t)day;
	date_time.hour = hour;
	date_time.minute = minute;
	date_time.second = second;
	date_time.dayOfWeek	= dayNames[dayOfWeek];
	date_time.monthOfYear = monthNames[month-1];
	
	return date_time;
}
void RTCSTM32::setDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{	
	normalizeDateTime(&year, &month, &day, &hour, &minute, &second);

	uint32_t counts = dateToDays(year, month, day) * SECONDS_PER_DAY;
	counts += (uint32_t)hour * 3600UL + (uint32_t)minute * 60UL + second;

	if((BKP->DR1 & 0x8000) > 0)
	{
		PWR_BackupAccessCmd(ENABLE);
		RTC_WaitForLastTask();
		RTC_SetCounter(counts);
		RTC_WaitForLastTask();
		PWR_BackupAccessCmd(DISABLE);
	}
}	

#ifdef __cplusplus
}
#endif
