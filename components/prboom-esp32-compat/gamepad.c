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
#include "m_cheat.h"
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
    uint8_t Volume;
    uint8_t Menu;
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

	{0x4000, &key_use},				//start
	{0x2000, &key_fire},			//circle
	{0x2000, &key_menu_enter},		//circle
	{0x8000, &key_loadgame},			//square
	{0x1000, &key_weapontoggle},	//triangle

	{0x8, &key_escape},				//menu
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
    state.Volume = !(gpio_get_level(ODROID_GAMEPAD_IO_VOLUME));
    state.Menu = !(gpio_get_level(ODROID_GAMEPAD_IO_MENU));

    state.A = !(gpio_get_level(ODROID_GAMEPAD_IO_A));
    state.B = !(gpio_get_level(ODROID_GAMEPAD_IO_B));

    //state.values[ODROID_INPUT_MENU] = !(gpio_get_level(ODROID_GAMEPAD_IO_MENU));
    //state.values[ODROID_INPUT_VOLUME] = !(gpio_get_level(ODROID_GAMEPAD_IO_VOLUME));


	int result = 0;

	if (!state.Up)
		result |= 0x10; //key_up

	if (!state.Down)
		result |= 0x40; //key_down

	if (!state.Left)
		result |= 0x80; //key_left

	if (!state.Right)
		result |= 0x20; //key_right

	if (!state.A)
		result |= 0x2000; //key_fire, key_menu_enter

	if (!state.Start)
		result |= 0x4000; //key_use

	if (!state.Menu)
		result |= 0x8; //key_escape

	if (!state.Select)
		result |= 0x1000; //key_weapontoggle

	if (!state.Volume)
		result |= 0x1; //key_map
	
	if (!state.B){
		result |= 0x200; //key_strafe
		result |= 0x100; //key_run
	}

	if(state.Menu && state.Up){
		//menu+up pressed at the same time
		char *code = "iddqd";
		int i;
		for(i=0; i<strlen(code); i++){
			M_FindCheats(code[i]);
		}
		result = 0;
	}
	if(state.Menu && state.Down){
		//menu+down pressed at the same time
		char *code = "idkfa";
		int i;
		for(i=0; i<strlen(code); i++){
			M_FindCheats(code[i]);
		}
		result = 0;
	}
	if(state.Menu && state.Left){
		//menu+left pressed at the same time
		char *code = "idclip";
		int i;
		for(i=0; i<strlen(code); i++){
			M_FindCheats(code[i]);
		}
		result = 0;
	}
	if(state.Menu && state.Right){
		//menu+right pressed at the same time
		char *code = "idbeholdh";
		int i;
		for(i=0; i<strlen(code); i++){
			M_FindCheats(code[i]);
		}
		result = 0;
	}
	if(state.Volume && state.Up){
		//volume+up pressed at the same time
		char *code = "idbeholdl";
		int i;
		for(i=0; i<strlen(code); i++){
			M_FindCheats(code[i]);
		}
		result = 0;
	}
	if(state.Volume && state.Down){
		//volume+down pressed at the same time
		char *code = "idbeholds";
		int i;
		for(i=0; i<strlen(code); i++){
			M_FindCheats(code[i]);
		}
		result = 0;
	}
	if(state.Volume && state.Left){
		//volume+left pressed at the same time
		char *code = "idbeholdi";
		int i;
		for(i=0; i<strlen(code); i++){
			M_FindCheats(code[i]);
		}
		result = 0;
	}
	if(state.Volume && state.Right){
		//volume+right pressed at the same time
		char *code = "idbeholdr";
		int i;
		for(i=0; i<strlen(code); i++){
			M_FindCheats(code[i]);
		}
		result = 0;
	}

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
