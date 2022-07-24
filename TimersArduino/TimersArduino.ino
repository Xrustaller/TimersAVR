/* -------------------------------------- Основные  настройки -------------------------------------- */

#define TIMER_MODE 2 // 1 - таймер, 2 - обратный таймер

#define GROUP_RELAY_MODE 1 // 1 - Полное включение, 2 - пульсация
#define GROUP_RELAY_MODE_PIN 2 // 1 - прямое реле, 2 - обратное реле

#define BTN_PIN_START 2 // Кнопка СТАРТ
#define BTN_PIN_RESET 3 // Кнопка СБРОС
#define BTN_PIN_MINUS 4 // Кнопка -
#define BTN_PIN_PLUS 5 // Кнопка +

#define FIRST_GROUP_RELAY_PIN_1 6
#define FIRST_GROUP_RELAY_PIN_2 7

#define BEEP_PIN 9 // Пин пьезо (при выборе 9 пина, 10 - недоступен из-за шим)

/* ------------------------------------------------------------------------------------------------- */

/* ---------------------------- Настройка обмена данными между модулями ---------------------------- */

#define BUS_ID 3 // Адрес (Таймер) 
#define PIN_TIMER_ON 13 // Вывод сигнала включения

/* ------------------------------------------------------------------------------------------------- */

/* ------------------------------------- Подключение библиотек ------------------------------------- */

#include <Wire.h>
#include <GyverButton.h>
#include <DTM1650.h>
#include <TIMECLASS.h>
#include <ModbusRtu.h>

/* ------------------------------------------------------------------------------------------------- */

/* -- Глобальные переменные -- */

// Кнопки управления
GButton button_start(BTN_PIN_START);
GButton button_reset(BTN_PIN_RESET);
GButton button_minus(BTN_PIN_MINUS);
GButton button_plus(BTN_PIN_PLUS);
// Экран
DTM1650 display;
// Работа с шиной
Modbus bus(BUS_ID, 0, 0);
int8_t state = 0;
uint16_t temp[2] = { 0, 0 };

bool is_start_timer = false;
bool is_timer_pause = false;
XLibs::TIMECLASS time_set(1, 0);

unsigned long timer;
XLibs::TIMECLASS time;

unsigned long timer_relay_pulse;
bool is_relay_pulse = false;

unsigned long tm;

void relay_on()
{
#if GROUP_RELAY_MODE == 1
	#if GROUP_RELAY_MODE_PIN == 1
	    digitalWrite(FIRST_GROUP_RELAY_PIN_1, HIGH);
	    digitalWrite(FIRST_GROUP_RELAY_PIN_2, HIGH);
	#else
	    digitalWrite(FIRST_GROUP_RELAY_PIN_1, LOW);
	    digitalWrite(FIRST_GROUP_RELAY_PIN_2, LOW);
	#endif
#endif
}

void relay_off()
{
#if GROUP_RELAY_MODE_PIN == 1
    digitalWrite(FIRST_GROUP_RELAY_PIN_1, LOW);
    digitalWrite(FIRST_GROUP_RELAY_PIN_2, LOW);
#else
    digitalWrite(FIRST_GROUP_RELAY_PIN_1, HIGH);
    digitalWrite(FIRST_GROUP_RELAY_PIN_2, HIGH);
#endif
}

void relay_pulse(const bool pulse)
{
    if (is_relay_pulse)
    {
        if (millis() - timer_relay_pulse > 1000)
        {
#if GROUP_RELAY_MODE == 2
#if GROUP_RELAY_MODE_PIN == 1
            digitalWrite(FIRST_GROUP_RELAY_PIN_1, LOW);
            digitalWrite(FIRST_GROUP_RELAY_PIN_2, LOW);
#else
            digitalWrite(FIRST_GROUP_RELAY_PIN_1, HIGH);
            digitalWrite(FIRST_GROUP_RELAY_PIN_2, HIGH);
#endif
#endif
            is_relay_pulse = false;
        }
    }
	
	if (pulse)
	{
#if GROUP_RELAY_MODE == 2
#if GROUP_RELAY_MODE_PIN == 1
        digitalWrite(FIRST_GROUP_RELAY_PIN_1, HIGH);
        digitalWrite(FIRST_GROUP_RELAY_PIN_2, HIGH);
#else
        digitalWrite(FIRST_GROUP_RELAY_PIN_1, LOW);
        digitalWrite(FIRST_GROUP_RELAY_PIN_2, LOW);
#endif
#endif

        is_relay_pulse = true;
        timer_relay_pulse = millis();
	}
}

void draw()
{
    display.write_longtime(time.GetMin(), time.GetSec());
}

void draw_set()
{
    display.write_time(time_set.GetMin(), time_set.GetSec());
}

void setup()
{
    button_start.setClickTimeout(50);
    button_reset.setClickTimeout(50);
  
    button_minus.setClickTimeout(200);
    button_plus.setClickTimeout(200);

    button_minus.setStepTimeout(10);
    button_plus.setStepTimeout(10);
	
    pinMode(FIRST_GROUP_RELAY_PIN_1, OUTPUT);
    pinMode(FIRST_GROUP_RELAY_PIN_2, OUTPUT);
    pinMode(PIN_TIMER_ON, OUTPUT);
    relay_off();

    Wire.begin();
    bus.begin(19200);
    
    display.init();
    display.set_brightness(DTM1650_BRIGHTNESS_MAX);
    draw_set();
}

void timer_set_plus()
{
    if (time_set.GetSec() >= 59 && time_set.GetMin() >= 99)
    {
        tone(BEEP_PIN, 2000, 100);
    }
    else
    {
        time_set.TickSec();
    }
    draw_set();
}

void timer_set_plus_5_min()
{
    if (time_set.GetMin() >= 95)
    {
        tone(BEEP_PIN, 2000, 100);
    }
    else
    {
        time_set.AddMin(5);
    }
    draw_set();
}

void timer_set_plus_10_min()
{
    if (time_set.GetMin() >= 90)
    {
        tone(BEEP_PIN, 2000, 100);
    }
    else
    {
        time_set.AddMin(10);
    }
    draw_set();
}

void timer_set_minus()
{
    if (time_set.GetSec() < 2 && time_set.GetMin() < 1)
    {
        tone(BEEP_PIN, 200, 100);
    }
    else
    {
        time_set.TickSecBack();
    }
    draw_set();
}

void timer_set_minus_5_min()
{
    if (!time_set.RemMin(5))
    {
	    tone(BEEP_PIN, 200, 100);
    }
	draw_set();
}

void timer_set_minus_10_min()
{
    if (!time_set.RemMin(10))
    {
        tone(BEEP_PIN, 200, 100);
    }
    draw_set();
}

void reset_timer()
{
    time.Clear();
    temp[1] = 0;
    draw_set();
    relay_off();
    if (!is_timer_pause) relay_pulse(true);
    is_start_timer = false;
    is_timer_pause = false;
    digitalWrite(PIN_TIMER_ON, LOW);
    tone(BEEP_PIN, 5000, 1000);
}

void button_tick()
{
    button_start.tick();
    button_reset.tick();
    button_minus.tick();
    button_plus.tick();

    if (is_start_timer)
    {
        if (button_start.isSingle() && is_timer_pause)
        {
            is_timer_pause = false;
            relay_on();
            relay_pulse(true);
            tone(BEEP_PIN, 3000, 100);
        }

        if (button_reset.isSingle() && !is_timer_pause)
        {
            is_timer_pause = true;
            relay_off();
            relay_pulse(true);
            tone(BEEP_PIN, 2000, 100);
        }
    	
        if (button_reset.isHolded())
        {
            reset_timer();
        }

        return;
    }
	
    if (button_start.isSingle() || button_start.isHold())
    {
#if TIMER_MODE == 1
        time.Clear();
#else
        time = time_set;
#endif
        temp[1] = time.ToSec();
        relay_on();
        relay_pulse(!is_timer_pause);
        is_start_timer = true;
        is_timer_pause = false;
        timer = millis();
        draw();
        digitalWrite(PIN_TIMER_ON, HIGH);
        tone(BEEP_PIN, 5000, 100);
    }
    
    if (button_reset.isHolded())
    {
        time_set.Set(1 , 0);
        draw_set();
        tone(BEEP_PIN, 200, 100);
    }

    if (button_minus.isSingle())
    {
        timer_set_minus();
    }
    if (button_minus.isDouble())
    {
        timer_set_minus_5_min();
    }
    if (button_minus.isTriple())
    {
        timer_set_minus_10_min();
    }
    if (button_minus.isHolded())
    {
        timer_set_minus();
    }

    if (button_plus.isSingle())
    {
        timer_set_plus();
    }
    if (button_plus.isHolded())
    {
        timer_set_plus();
    }
    if (button_plus.isDouble())
    {
        timer_set_plus_5_min();
    }
    if (button_plus.isTriple())
    {
        timer_set_plus_10_min();
    }

    if (button_minus.isStep())
    {
        timer_set_minus();
    }
    if (button_plus.isStep())
    {
        timer_set_plus();
    }
}

void timer_tick()
{
#if TIMER_MODE == 1 // таймер
    if (time.GetSec() >= time_set.GetSec() && time.GetMin() >= time_set.GetMin())
    {
        reset_timer();
    }
    else
    {
        if (millis() - timer > 1000) {
            timer = millis();
            time.TickSec();
            draw();
            temp[1] = time.ToSec();
        }
    }
#else
    if (millis() - timer > 1000)
    {
        timer = millis();
        if (time.TickSecBack())
        {
	        draw();
            temp[1] = time.ToSec();
        }
        else
        {
	        reset_timer();
        }
    }
#endif
    
}

void loop()
{
    state = bus.poll(temp, 2);

    button_tick();
	
	if (is_start_timer && !is_timer_pause)
    {
		timer_tick();
    }

    relay_pulse(false);
    if (millis() - tm > 3000)
    {
        tm = millis();
    }
}
