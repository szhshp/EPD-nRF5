#include "fonts.h"
#include "Lunar.h"
#include "GUI.h"
#include <stdio.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define GFX_printf_styled(gfx, fg, bg, font, ...) \
            GFX_setTextColor(gfx, fg, bg);        \
            GFX_setFont(gfx, font);               \
            GFX_printf(gfx, __VA_ARGS__);

typedef struct {
    uint8_t month;
    uint8_t day;
    char name[10]; // 3x3+1
} Festival;

static const Festival festivals[] = {
    { 1,  1, "元旦节"},
    { 2, 14, "情人节"},
    { 3,  8, "妇女节"},
    { 3, 12, "植树节"},
    { 4,  1, "愚人节"},
    { 5,  1, "劳动节"},
    { 5,  4, "青年节"},
    { 6,  1, "儿童节"},
    { 7,  1, "建党节"},
    { 8,  1, "建军节"},
    { 9, 10, "教师节"},
    {10,  1, "国庆节"},
    {11,  1, "万圣节"},
    {12, 24, "平安夜"},
    {12, 25, "圣诞节"},
};

static const Festival festivals_lunar[] = {
    { 1,  1, "春节"  },
    { 1, 15, "元宵节"},
    { 2,  2, "龙抬头"},
    { 5,  5, "端午节"},
    { 7,  7, "七夕节"},
    { 7, 15, "中元节"},
    { 8, 15, "中秋节"},
    { 9,  9, "重阳节"},
    {10,  1, "寒衣节"},
    {12,  8, "腊八节"},
    {12, 30, "除夕"  },
};

// 放假和调休数据，每年更新
#define HOLIDAY_YEAR 2025
static const uint16_t holidays[] = {
    0x0101, 0x111A, 0x011C, 0x011D, 0x011E, 0x011F, 0x0201, 0x0202,
    0x0202, 0x0203, 0x0204, 0x1208, 0x0404, 0x0405, 0x0406, 0x141B,
    0x0501, 0x0502, 0x0503, 0x0504, 0x0505, 0x051F, 0x0601, 0x0602,
    0x191C, 0x0A01, 0x0A02, 0x0A03, 0x0A04, 0x0A05, 0x0A06, 0x0A07,
    0x0A08, 0x1A0B,
};

static bool GetHoliday(uint8_t mon, uint8_t day, bool *work)
{
    for (uint8_t i = 0; i < ARRAY_SIZE(holidays); i++) {
        if (((holidays[i] >> 8) & 0xF) == mon && (holidays[i] & 0xFF) == day) {
            *work = ((holidays[i] >> 12) & 0xF) > 0;
            return true;
        }
    }
    return false;
}

static bool GetFestival(uint16_t year, uint8_t mon, uint8_t day, uint8_t week,
                        struct Lunar_Date *Lunar, char *festival)
{
    // 农历节日
    for (uint8_t i = 0; i < ARRAY_SIZE(festivals_lunar); i++) {
        if (Lunar->Month == festivals_lunar[i].month && Lunar->Date == festivals_lunar[i].day) {
            strcpy(festival, festivals_lunar[i].name);
            return true;
        }
    }

    // 除夕：春节前一天（12/29 或 12/30），12/30 已在上面判断
    if (Lunar->Month == 12 && Lunar->Date == 29) {
        struct Lunar_Date nextLunar;
        struct devtm tm = {year, mon, day, 0, 0, 0, week};
        transformTime(transformTimeStruct(&tm) + 86400, &tm);
        LUNAR_SolarToLunar(&nextLunar, tm.tm_year + YEAR0, tm.tm_mon + 1, tm.tm_mday);
        if (nextLunar.Month == 1 && nextLunar.Date == 1) {
            strcpy(festival, "除夕");
            return true;
        }
    }
    // 母亲节: 五月第二个星期日
    if (mon == 5 && week == 0 && day >= 8 && day <= 14) {
        strcpy(festival, "母亲节");
        return true;
    }
    // 父亲节: 六月第三个星期日
    if (mon == 6 && week == 0 && day >= 15 && day <= 21) {
        strcpy(festival, "父亲节");
        return true;
    }
    // 感恩节：十一月第四个星期四
    if (mon == 11 && week == 4 && day >= 22 && day <= 28) {
        strcpy(festival, "感恩节");
        return true;
    }

    // 公历节日
    for (uint8_t i = 0; i < ARRAY_SIZE(festivals); i++) {
        if (mon == festivals[i].month && day == festivals[i].day) {
            strcpy(festival, festivals[i].name);
            return true;
        }
    }

    // 二十四节气
    uint8_t JQdate;
    if (GetJieQi(year, mon, day, &JQdate) && JQdate == day) {
        uint8_t JQ = (mon - 1) * 2;
        if (day >= 15) JQ++;
        strcpy(festival, JieQiStr[JQ]);
        if (JQ == 6) // 清明
            strcat(festival, "节");
            
        return true;
    }

    return false;
}

static void DrawTimeSyncTip(Adafruit_GFX *gfx)
{
    GFX_setFont(gfx, u8g2_font_wqy12_t_lunar);
    GFX_fillRect(gfx, gfx->_width / 2 - 100, gfx->_height / 2 - 25, 200, 50, GFX_WHITE);
    GFX_drawRoundRect(gfx, gfx->_width / 2 - 100, gfx->_height / 2 - 25, 200, 50, 5, GFX_BLACK);
    GFX_setTextColor(gfx, GFX_RED, GFX_WHITE);
    GFX_setCursor(gfx, 149, 145);
    GFX_printf(gfx, "SYNC TIME!");
    GFX_setTextColor(gfx, GFX_BLACK, GFX_WHITE);
    GFX_setCursor(gfx, 110, 164);
    GFX_setFont(gfx, u8g2_font_wqy9_t_lunar);
    GFX_printf(gfx, "https://tsl0922.github.io/EPD-nRF5");
}

static void DrawBattery(Adafruit_GFX *gfx, int16_t x, int16_t y, float voltage)
{
    uint8_t level = (uint8_t)(voltage * 100 / 4.2);
    GFX_setCursor(gfx, x - 26, y + 9);
    GFX_setFont(gfx, u8g2_font_wqy9_t_lunar);
    GFX_printf(gfx, "%.1fV", voltage);
    GFX_fillRect(gfx, x, y, 20, 10, GFX_WHITE);
    GFX_drawRect(gfx, x, y, 20, 10, GFX_BLACK);
    GFX_fillRect(gfx, x + 20, y + 4, 2, 2, GFX_BLACK);
    GFX_fillRect(gfx, x + 2, y + 2, 16 * level / 100, 6, GFX_BLACK);
}

static void DrawTemperature(Adafruit_GFX *gfx, int16_t x, int16_t y, int8_t temp)
{
    GFX_setCursor(gfx, x, y);
    GFX_setFont(gfx, u8g2_font_wqy9_t_lunar);
    GFX_printf(gfx, "%d℃", temp);
}

static void DrawDate(Adafruit_GFX *gfx, int16_t x, int16_t y, tm_t *tm)
{
    GFX_setCursor(gfx, x, y);
    GFX_printf_styled(gfx, GFX_RED, GFX_WHITE, u8g2_font_helvB18_tn, "%d", tm->tm_year + YEAR0);
    GFX_printf_styled(gfx, GFX_BLACK, GFX_WHITE, u8g2_font_wqy12_t_lunar, "年");
    GFX_printf_styled(gfx, GFX_RED, GFX_WHITE, u8g2_font_helvB18_tn, "%02d", tm->tm_mon + 1);
    GFX_printf_styled(gfx, GFX_BLACK, GFX_WHITE, u8g2_font_wqy12_t_lunar, "月");
    GFX_printf_styled(gfx, GFX_RED, GFX_WHITE, u8g2_font_helvB18_tn, "%02d", tm->tm_mday);
    GFX_printf_styled(gfx, GFX_BLACK, GFX_WHITE, u8g2_font_wqy12_t_lunar, "日 ");
}

static void DrawDateHeader(Adafruit_GFX *gfx, int16_t x, int16_t y, tm_t *tm, struct Lunar_Date *Lunar, gui_data_t *data)
{
    DrawDate(gfx, x, y, tm);
    GFX_setFont(gfx, u8g2_font_wqy9_t_lunar);
    GFX_printf(gfx, "星期%s", Lunar_DayString[tm->tm_wday]);

    DrawBattery(gfx, 366, 6, data->voltage);

    GFX_setCursor(gfx, x + (Lunar->IsLeap ? 266 : 272), y);
    GFX_printf(gfx, "%s%s%s %s%s", Lunar_MonthLeapString[Lunar->IsLeap], Lunar_MonthString[Lunar->Month],
                     Lunar_DateString[Lunar->Date], Lunar_StemStrig[LUNAR_GetStem(Lunar)],
                     Lunar_BranchStrig[LUNAR_GetBranch(Lunar)]);
    GFX_setTextColor(gfx, GFX_RED, GFX_WHITE);
    GFX_printf(gfx, "%s", Lunar_ZodiacString[LUNAR_GetZodiac(Lunar)]);
    GFX_setTextColor(gfx, GFX_BLACK, GFX_WHITE);
    GFX_printf(gfx, "年");
}

static void DrawWeekHeader(Adafruit_GFX *gfx, int16_t x, int16_t y)
{
    GFX_fillRect(gfx, x, y, 380, 24, GFX_RED);
    GFX_fillRect(gfx, x + 50, y, 280, 24, GFX_BLACK);
    GFX_setFont(gfx, u8g2_font_wqy9_t_lunar);
    for (int i = 0; i < 7; i++) {
        GFX_setTextColor(gfx, GFX_WHITE, (i > 0 && i < 6) ? GFX_BLACK : GFX_RED);
        GFX_setCursor(gfx, x + 15 + i * 55, y + 16);
        GFX_printf(gfx, "%s", Lunar_DayString[i]);
    }
}

static void DrawMonthDays(Adafruit_GFX *gfx, tm_t *tm, struct Lunar_Date *Lunar)
{
    uint8_t firstDayWeek = get_first_day_week(tm->tm_year + YEAR0, tm->tm_mon + 1);
    uint8_t monthMaxDays = thisMonthMaxDays(tm->tm_year + YEAR0, tm->tm_mon + 1);
    uint8_t monthDayRows = 1 + (monthMaxDays - (7 - firstDayWeek) + 6) / 7;

    for (uint8_t i = 0; i < monthMaxDays; i++) {
        uint16_t year = tm->tm_year + YEAR0;
        uint8_t month = tm->tm_mon + 1;
        uint8_t day = i + 1;

        int16_t week = (firstDayWeek + i) % 7;
        bool weekend = (week  == 0) || (week == 6);

        int16_t x = 22 + week * 55;
        int16_t y = (monthDayRows > 5 ? 69 : 72) + (firstDayWeek + i) / 7 * (monthDayRows > 5 ? 39 : 48);

        if (day == tm->tm_mday) {
            GFX_fillCircle(gfx, x + 11, y + (monthDayRows > 5 ? 10 : 12), 22, GFX_RED);
            GFX_setTextColor(gfx, GFX_WHITE, GFX_RED);
        } else {
            GFX_setTextColor(gfx, weekend ? GFX_RED : GFX_BLACK, GFX_WHITE);
        }

        GFX_setFont(gfx, u8g2_font_helvB14_tn);
        GFX_setCursor(gfx, x + (day < 10 ? 6 : 2), y + 10);
        GFX_printf(gfx, "%d", day);
        
        GFX_setFont(gfx, u8g2_font_wqy9_t_lunar);
        LUNAR_SolarToLunar(Lunar, year, month, day);

        char festival[10] = {0};
        if (GetFestival(year, month, day, week, Lunar, festival)) {
            if (day != tm->tm_mday) GFX_setTextColor(gfx, GFX_RED, GFX_WHITE);
            GFX_setCursor(gfx, strlen(festival) > 6 ? x - 6 : x, y + 24);
            GFX_printf(gfx, "%s", festival);
        } else {
            GFX_setCursor(gfx, x, y + 24);
            if (Lunar->Date == 1)
                GFX_printf(gfx, "%s", Lunar_MonthString[Lunar->Month]);
            else
                GFX_printf(gfx, "%s", Lunar_DateString[Lunar->Date]);
        }
        bool work = false;
        if (year == HOLIDAY_YEAR && GetHoliday(month, day, &work)) {
            if (day == tm->tm_mday) {
                GFX_fillCircle(gfx, x + 30, y + 1, 8, GFX_WHITE);
                GFX_drawCircle(gfx, x + 30, y + 1, 8, GFX_RED);
            }
            GFX_setTextColor(gfx, work ? GFX_BLACK : GFX_RED, GFX_WHITE);
            GFX_setCursor(gfx, x + 25, y + 6);
            GFX_printf(gfx, "%s", work ? "班" : "休");
        }
    }
}

static void DrawCalendar(Adafruit_GFX *gfx, tm_t *tm, struct Lunar_Date *Lunar, gui_data_t *data)
{
    DrawDateHeader(gfx, 10, 28, tm, Lunar, data);
    DrawWeekHeader(gfx, 10, 32);
    DrawMonthDays(gfx, tm, Lunar);
}

/* Routine to Draw Large 7-Segment formated number
   Contributed by William Zaggle.

   int n - The number to be displayed
   int xLoc = The x location of the upper left corner of the number
   int yLoc = The y location of the upper left corner of the number
   int cS = The size of the number. 
   fC is the foreground color of the number
   bC is the background color of the number (prevents having to clear previous space)
   nD is the number of digit spaces to occupy (must include space for minus sign for numbers < 0).

   width: nD*(11*cS+2)-2*cS
   height: 20*cS+4

   https://forum.arduino.cc/t/fast-7-segment-number-display-for-tft/296619/4
*/
static void Draw7Number(Adafruit_GFX *gfx, int n, unsigned int xLoc, unsigned int yLoc, char cS, unsigned int fC, unsigned int bC, char nD) {
    unsigned int num=abs(n),i,t,w,col,h,a,b,j=1,d=0,S2=5*cS,S3=2*cS,S4=7*cS,x1=cS+1,x2=S3+S2+1,y1=yLoc+x1,y3=yLoc+S3+S4+1;
    unsigned int seg[7][3]={{x1,yLoc,1},{x2,y1,0},{x2,y3+x1,0},{x1,(2*y3)-yLoc,1},{0,y3+x1,0},{0,y1,0},{x1,y3,1}};
    unsigned char nums[12]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,0x00,0x40},c=(c=abs(cS))>10?10:(c<1)?1:c,cnt=(cnt=abs(nD))>10?10:(cnt<1)?1:cnt;
    for (xLoc+=cnt*(d=S2+(3*S3)+2);cnt>0;cnt--){
      for (i=(num>9)?num%10:((!cnt)&&(n<0))?11:((nD<0)&&(!num))?10:num,xLoc-=d,num/=10,j=0;j<7;++j){
        col=(nums[i]&(1<<j))?fC:bC;
        if (seg[j][2])for(w=S2,t=seg[j][1]+S3,h=seg[j][1]+cS,a=xLoc+seg[j][0]+cS,b=seg[j][1];b<h;b++,a--,w+=2)GFX_drawFastHLine(gfx,a,b,w,col);
        else for(w=S4,t=xLoc+seg[j][0]+S3,h=xLoc+seg[j][0]+cS,b=xLoc+seg[j][0],a=seg[j][1]+cS;b<h;b++,a--,w+=2)GFX_drawFastVLine(gfx,b,a,w,col);
        for (;b<t;b++,a++,w-=2)seg[j][2]?GFX_drawFastHLine(gfx,a,b,w,col):GFX_drawFastVLine(gfx,b,a,w,col);
        }
    }
}

static void DrawTime(Adafruit_GFX *gfx, tm_t *tm, int16_t x, int16_t y, uint16_t cS, uint16_t nD)
{
    Draw7Number(gfx, tm->tm_hour, x, y, cS, GFX_BLACK, GFX_WHITE, nD);
    x += (nD*(11*cS+2)-2*cS) + 2*cS;
    GFX_fillRect(gfx, x, y + 4.5*cS+1, 2*cS, 2*cS, GFX_BLACK);
    GFX_fillRect(gfx, x, y + 13.5*cS+3, 2*cS, 2*cS, GFX_BLACK);
    x += 4*cS;
    Draw7Number(gfx, tm->tm_min, x, y, cS, GFX_BLACK, GFX_WHITE, nD);
}

static void DrawClock(Adafruit_GFX *gfx, tm_t *tm, struct Lunar_Date *Lunar, gui_data_t *data)
{
    DrawDate(gfx, 40, 36, tm);
    GFX_setCursor(gfx, 40, 58);
    GFX_setFont(gfx, u8g2_font_wqy9_t_lunar);
    GFX_printf(gfx, "星期%s", Lunar_DayString[tm->tm_wday]);
    GFX_setCursor(gfx, 138, 58);
    GFX_printf(gfx, "%s%s%s", Lunar_MonthLeapString[Lunar->IsLeap], Lunar_MonthString[Lunar->Month],
        Lunar_DateString[Lunar->Date]);

    DrawBattery(gfx, 330, 25, data->voltage);
    DrawTemperature(gfx, 330, 58, data->temperature);

    GFX_drawFastHLine(gfx, 30, 68, 330, GFX_BLACK);
    DrawTime(gfx, tm, 70, 98, 5, 2);
    GFX_drawFastHLine(gfx, 30, 232, 330, GFX_BLACK);

    GFX_setCursor(gfx, 40, 275);
    GFX_setFont(gfx, u8g2_font_wqy12_t_lunar);
    GFX_printf(gfx, "%s%s%s年", Lunar_StemStrig[LUNAR_GetStem(Lunar)], Lunar_BranchStrig[LUNAR_GetBranch(Lunar)],
        Lunar_ZodiacString[LUNAR_GetZodiac(Lunar)]);

    uint8_t day = 0;
    uint8_t JQday = GetJieQiStr(tm->tm_year + YEAR0, tm->tm_mon + 1, tm->tm_mday, &day);
    if (day == 0) {
        GFX_setCursor(gfx, 320, 275);
        GFX_printf(gfx, "%s", JieQiStr[JQday % 24]);
    } else {
        GFX_setCursor(gfx, 300, 265);
        GFX_printf(gfx, "离%s", JieQiStr[JQday % 24]);
        GFX_setCursor(gfx, 290, 285);
        GFX_printf(gfx, "还有%d天", day);
    }
}

void DrawGUI(gui_data_t *data, buffer_callback draw, display_mode_t mode)
{
    tm_t tm = {0};
    struct Lunar_Date Lunar;

    transformTime(data->timestamp, &tm);

    Adafruit_GFX gfx;

    if (data->color == 2)
      GFX_begin_3c(&gfx, data->width, data->height, PAGE_HEIGHT);
    else if (data->color == 3)
      GFX_begin_4c(&gfx, data->width, data->height, PAGE_HEIGHT);
    else
      GFX_begin(&gfx, data->width, data->height, PAGE_HEIGHT);

    GFX_firstPage(&gfx);
    do {
        GFX_fillScreen(&gfx, GFX_WHITE);

        LUNAR_SolarToLunar(&Lunar, tm.tm_year + YEAR0, tm.tm_mon + 1, tm.tm_mday);

        switch (mode) {
            case MODE_CALENDAR:
                DrawCalendar(&gfx, &tm, &Lunar, data);
                break;
            case MODE_CLOCK:
                DrawClock(&gfx, &tm, &Lunar, data);
                break;
            default:
                break;
        }
        if ((mode == MODE_CALENDAR || mode == MODE_CLOCK) &&
            (tm.tm_year + YEAR0 == 2025 && tm.tm_mon + 1 == 1)) {
            DrawTimeSyncTip(&gfx);
        }
    } while(GFX_nextPage(&gfx, draw));

    GFX_end(&gfx);
}
