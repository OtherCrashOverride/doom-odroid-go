// Copyright 2016-2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <stdlib.h>

#include "doomdef.h"
#include "doomtype.h"
#include "m_argv.h"
#include "d_event.h"
#include "g_game.h"
#include "d_main.h"
#include "gamepad.h"
#include "lprintf.h"

#include "psxcontroller.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <driver/adc.h>

#define ODROID_GAMEPAD_IO_X ADC1_CHANNEL_6
#define ODROID_GAMEPAD_IO_Y ADC1_CHANNEL_7
#define ODROID_GAMEPAD_IO_SELECT GPIO_NUM_27
#define ODROID_GAMEPAD_IO_START GPIO_NUM_39
#define ODROID_GAMEPAD_IO_A GPIO_NUM_32
#define ODROID_GAMEPAD_IO_B GPIO_NUM_33
#define ODROID_GAMEPAD_IO_MENU GPIO_NUM_13
#define ODROID_GAMEPAD_IO_VOLUME GPIO_NUM_0


typedef struct
{
    uint8_t Up;
    uint8_t Right;
    uint8_t Down;
    uint8_t Left;
    uint8_t Select;
    uint8_t Start;
    uint8_t A;
    uint8_t B;
} JoystickState;


//The gamepad uses keyboard emulation, but for compilation, these variables need to be placed
//somewhere. THis is as good a place as any.
int usejoystick=0;
int joyleft, joyright, joyup, joydown;


//atomic, for communication between joy thread and main game thread
volatile int joyVal=0;

typedef struct {
	int ps2mask;
	int *key;
} JsKeyMap;

//Mappings from PS2 buttons to keys
static const JsKeyMap keymap[]={
	{0x10, &key_up},
	{0x40, &key_down},
	{0x80, &key_left},
	{0x20, &key_right},

	{0x4000, &key_use},				//cross
	{0x2000, &key_fire},			//circle
	{0x2000, &key_menu_enter},		//circle
	{0x8000, &key_pause},			//square
	{0x1000, &key_weapontoggle},	//triangle

	{0x8, &key_escape},				//start
	{0x1, &key_map},				//select

	{0x400, &key_strafeleft},		//L1
	{0x100, &key_speed},			//L2
	{0x800, &key_straferight},		//R1
	{0x200, &key_strafe},			//R2

	{0, NULL},
};

int JoystickRead()
{
	JoystickState state;


    int joyX = adc1_get_raw(ODROID_GAMEPAD_IO_X);
    int joyY = adc1_get_raw(ODROID_GAMEPAD_IO_Y);

    if (joyX > 2048 + 1024)
    {
        state.Left = 1;
        state.Right = 0;
    }
    else if (joyX > 1024)
    {
        state.Left = 0;
        state.Right = 1;
    }
    else
    {
        state.Left = 0;
        state.Right = 0;
    }

    if (joyY > 2048 + 1024)
    {
        state.Up = 1;
        state.Down = 0;
    }
    else if (joyY > 1024)
    {
        state.Up = 0;
        state.Down = 1;
    }
    else
    {
        state.Up = 0;
        state.Down = 0;
    }

    state.Select = !(gpio_get_level(ODROID_GAMEPAD_IO_SELECT));
    state.Start = !(gpio_get_level(ODROID_GAMEPAD_IO_START));

    state.A = !(gpio_get_level(ODROID_GAMEPAD_IO_A));
    state.B = !(gpio_get_level(ODROID_GAMEPAD_IO_B));

    //state.values[ODROID_INPUT_MENU] = !(gpio_get_level(ODROID_GAMEPAD_IO_MENU));
    //state.values[ODROID_INPUT_VOLUME] = !(gpio_get_level(ODROID_GAMEPAD_IO_VOLUME));


	int result = 0;

	if (!state.Up)
		result |= 0x10;

	if (!state.Down)
		result |= 0x40;

	if (!state.Left)
		result |= 0x80;

	if (!state.Right)
		result |= 0x20;

	if (!state.A)
		result |= 0x2000;

	if (!state.B)
		result |= 0x4000;

	if (!state.Start)
		result |= 0x8;

	if (!state.Select)
		result |= 0x1;

	return result;
}

void gamepadPoll(void)
{
	static int oldPollJsVal=0xffff;
	int newJoyVal=joyVal;
	event_t ev;

	for (int i=0; keymap[i].key!=NULL; i++) {
		if ((oldPollJsVal^newJoyVal)&keymap[i].ps2mask) {
			ev.type=(newJoyVal&keymap[i].ps2mask)?ev_keyup:ev_keydown;
			ev.data1=*keymap[i].key;
			D_PostEvent(&ev);
		}
	}

	oldPollJsVal=newJoyVal;
}



void jsTask(void *arg) {
	int oldJoyVal=0xFFFF;
	printf("Joystick task starting.\n");
	while(1) {
		vTaskDelay(20/portTICK_PERIOD_MS);
		joyVal=JoystickRead();
//		if (joyVal!=oldJoyVal) printf("Joy: %x\n", joyVal^0xffff);
		oldJoyVal=joyVal;
	}
}

void gamepadInit(void)
{
	lprintf(LO_INFO, "gamepadInit: Initializing game pad.\n");
}

void JoystickInit()
{
    gpio_set_direction(ODROID_GAMEPAD_IO_SELECT, GPIO_MODE_INPUT);
	gpio_set_pull_mode(ODROID_GAMEPAD_IO_SELECT, GPIO_PULLUP_ONLY);

	gpio_set_direction(ODROID_GAMEPAD_IO_START, GPIO_MODE_INPUT);

	gpio_set_direction(ODROID_GAMEPAD_IO_A, GPIO_MODE_INPUT);
	gpio_set_pull_mode(ODROID_GAMEPAD_IO_A, GPIO_PULLUP_ONLY);

    gpio_set_direction(ODROID_GAMEPAD_IO_B, GPIO_MODE_INPUT);
	gpio_set_pull_mode(ODROID_GAMEPAD_IO_B, GPIO_PULLUP_ONLY);

	adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(ODROID_GAMEPAD_IO_X, ADC_ATTEN_11db);
	adc1_config_channel_atten(ODROID_GAMEPAD_IO_Y, ADC_ATTEN_11db);

	gpio_set_direction(ODROID_GAMEPAD_IO_MENU, GPIO_MODE_INPUT);
	gpio_set_pull_mode(ODROID_GAMEPAD_IO_MENU, GPIO_PULLUP_ONLY);

    gpio_set_direction(ODROID_GAMEPAD_IO_VOLUME, GPIO_MODE_INPUT);
}

void jsInit() {
	//Starts the js task
	JoystickInit();
	xTaskCreatePinnedToCore(&jsTask, "js", 5000, NULL, 7, NULL, 0);
}
