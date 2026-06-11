> ***Worked by***  刘翊凡

# Task2 带按钮的温湿度计

**设计**

**1.**为Task1的温湿度计配备按钮，具体按钮规则为**：**

> SW9：开机，显示`0000`，并亮白光
>
> SW8：关机，显示`FFFF`，不亮光
>
> SW1：显示温度，并亮红光
>
> SW2：显示湿度，并亮绿光
>
> SW3：显示光照度，并亮蓝光
>
> SW4：复位，显示`0000`，并亮白光

**实现**

> 获取温湿度的部分沿袭了Task1

**1.主函数状态变化逻辑：**

```C
if(s1_key_addr.flag)
{    
    //获取按键值
    key_value = s1_key_scan(s1_key_addr.periph,s1_key_addr.addr);
    if(key_value != SWN)
    {
        sprintf((char *)print_buffer,(const char*)"key value = %d\r\n",key_value); 
        debug_printf(EVAL_COM0,(char*)print_buffer);
        //获取当前状态
        get_display_state();
    }   
}

if(system_state == 1)
{
    switch (display_state)
    {
        case 1://显示温度
            display_temperature();
            e1_rgb_control(e1_rgb_led_addr.periph,e1_rgb_led_addr.addr,60,0,0);
            break;
        case 2://显示湿度
            e1_rgb_control(e1_rgb_led_addr.periph,e1_rgb_led_addr.addr,0,60,0);
            display_humidity();
            break;
        case 3://显示光照度
            e1_rgb_control(e1_rgb_led_addr.periph,e1_rgb_led_addr.addr,0,0,60);
            display_sunshine();
            break;
        case 4://复位
            e1_rgb_control(e1_rgb_led_addr.periph,e1_rgb_led_addr.addr,60,60,60);
            e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, 0, 0, 0, 0, tmp);
    }
}
else
{
    //关机状态，显示FFFF
    e1_rgb_control(e1_rgb_led_addr.periph,e1_rgb_led_addr.addr,0,0,0);
    e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, 15, 15, 15, 15, tmp);
}
```

**2.显示状态（display_state）及系统开关机状态（system_state）获取：**

```C
void get_display_state(){

    if(key_value == SW1)
    { //temperature
        display_state = 1;
    }
    else if(key_value == SW2)
    { //humidity
        display_state = 2;
    }
    else if(key_value == SW3)
    { //sunshine
        display_state = 3;
    }
    else if(key_value == SW4)
    { //reset
        display_state = 4;
    }
    else if(key_value == SW8)
    {
        system_state = 0; // swtich off
        display_state = 4;// 关机时将显示状态置为复位
    }
    else if(key_value == SW9)
    {
        system_state = 1; // switch on
    }
}
```

**遇到的问题**

1.编写代码时E2子板忘记初始化，在测试时发现始终无数值显示并debug检测参数值后发现了问题，并做出修正

2.在测试中我们发现，E1子板的LED灯和数码管的显示都是"***一次调用，持久显示*** "的，不需要我们在每次循环都去调用