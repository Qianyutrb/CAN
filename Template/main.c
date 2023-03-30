/*!
    \file    main.c
    \brief   communication_among_CANS in normal mode
    
    \version 2017-02-10, V1.0.0, firmware for GD32F30x
    \version 2018-10-10, V1.1.0, firmware for GD32F30x
    \version 2018-12-25, V2.0.0, firmware for GD32F30x
    \version 2020-09-30, V2.1.0, firmware for GD32F30x
*/

/*
    Copyright (c) 2020, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this 
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice, 
       this list of conditions and the following disclaimer in the documentation 
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors 
       may be used to endorse or promote products derived from this software without 
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
OF SUCH DAMAGE.
*/

#include "gd32f30x.h"
#include <stdio.h>
#include "gd32f307c_eval.h"

/* select CAN baudrate */
/* 1MBps */
#define CAN_BAUDRATE  1000
/* 500kBps */
/* #define CAN_BAUDRATE  500 */
/* 250kBps */
/* #define CAN_BAUDRATE  250 */
/* 125kBps */
/* #define CAN_BAUDRATE  125 */
/* 100kBps */ 
/* #define CAN_BAUDRATE  100 */
/* 50kBps */ 
/* #define CAN_BAUDRATE  50 */
/* 20kBps */ 
/* #define CAN_BAUDRATE  20 */

FlagStatus can0_receive_flag;
FlagStatus can1_receive_flag;
FlagStatus can0_error_flag;
FlagStatus can1_error_flag;

can_trasnmit_message_struct transmit_message;
can_receive_message_struct receive_message;

void nvic_config(void);
void led_config(void);
void can_gpio_config(void);
void can_config(void);
/* ����һ��Ƕ��ʽ C ���Դ��룬������ GD32F30x ˫ CAN �����Ͻ���ͨ�Ų��ԡ�
�ô�������˶� GPIO��USART��������LED �ȵ����úͳ�ʼ�����Լ� CAN ���ߵ����á���Ϣ���ͺͽ��ա�
������ʹ����ѭ������ⰴ����״̬��������Ӧ�����ݡ�
�����յ� CAN �����ϵ�����ʱ������Ὣ���ӡ���������� LED �Ƶ�״̬��
���⣬�ô��뻹������һЩ��������ƣ����緢�ͳ�ʱ����� */

int main(void)
{
    uint8_t i = 0;                // ����һ��8λ�޷�������i����ʼ��Ϊ0����ѭ������
    uint32_t timeout = 0xFFFF;    // ����һ��32λ�޷�������timeout����ʼ��Ϊ0xFFFF��ʱʱ�䣬���ڵȴ�CANģ���ʼ�����
    uint8_t transmit_mailbox = 0; // ����һ��8λ�޷�������transmit_mailbox����ʼ��Ϊ0�����������ţ����ڱ�ʶ���͵�CAN��Ϣ���ĸ�������

    can0_receive_flag = RESET; // ��CAN0���ձ�־����
    can1_receive_flag = RESET; // ��CAN1���ձ�־����
    can0_error_flag = RESET;   // ��CAN0�����־����
    can1_error_flag = RESET;   // ��CAN1�����־����

    // ����GPIO����
    can_gpio_config();

    // �����ж�������
    nvic_config();

    /* ����USART����ͨ�� */
    gd_eval_com_init(EVAL_COM0);

    /* ����Wakeup����Tamper�� */
    gd_eval_key_init(KEY_WAKEUP, KEY_MODE_GPIO);
    gd_eval_key_init(KEY_TAMPER, KEY_MODE_GPIO);

    printf("\r\nGD32F30x dual CAN test, please press Wakeup key or Tamper key to start communication!\r\n");
    /* ����LED�� */
    led_config();
    gd_eval_led_off(LED2); // �ر�LED2
    gd_eval_led_off(LED3); // �ر�LED3
    gd_eval_led_off(LED4); // �ر�LED4
    gd_eval_led_off(LED5); // �ر�LED5

    /* ��ʼ��CAN�͹����� */
    can_config();
    /* enable can receive FIFO0 not empty interrupt */
    can_interrupt_enable(CAN0, CAN_INT_RFNE0); // ʹ��CAN0�Ľ���FIFO0�ǿ��ж�
    can_interrupt_enable(CAN1, CAN_INT_RFNE0); // ʹ��CAN1�Ľ���FIFO0�ǿ��ж�

    /* initialize transmit message */
    transmit_message.tx_sfid = 0x7ab;     // ��ʼ��������Ϣ�ı�׼֡ID
    transmit_message.tx_efid = 0x00;      // ��ʼ����չ֡ID
    transmit_message.tx_ft = CAN_FT_DATA; // ��ʼ����������֡����
    transmit_message.tx_ff = CAN_FF_STANDARD;
    transmit_message.tx_dlen = 8; // �������ݳ���8�ֽ�
    /* �������� */
    transmit_message.tx_data[0] = 0x00;
    transmit_message.tx_data[1] = 0xA1;
    transmit_message.tx_data[2] = 0xA2;
    transmit_message.tx_data[3] = 0xA3;
    transmit_message.tx_data[4] = 0xA4;
    transmit_message.tx_data[5] = 0xA5;
    transmit_message.tx_data[6] = 0xA6;
    transmit_message.tx_data[7] = 0xA7;

    while (1)
    {
        /* ����Ƿ���Tamper���� */
        if (0 == gd_eval_key_state_get(KEY_TAMPER)) // �������Tamper����
        {
            transmit_message.tx_data[0] = 0x55;            // ���÷������ݵ�һ���ֽ�Ϊ0x55
            transmit_message.tx_data[1] = 0xAA;            // ���÷������ݵڶ����ֽ�Ϊ0xAA
            printf("\r\n can0 transmit data:");            // ��ӡ��ʾ��Ϣ
            for (i = 0; i < transmit_message.tx_dlen; i++) // ѭ����ӡ�������ݵ�ÿһ���ֽ�
            {
                printf(" %02x", transmit_message.tx_data[i]);
            }

            /* ������Ϣ */
            transmit_mailbox = can_message_transmit(CAN0, &transmit_message);
            /* �ȴ�������� */
            timeout = 0xFFFF;
            while ((CAN_TRANSMIT_OK != can_transmit_states(CAN0, transmit_mailbox)) && (0 != timeout))
            {
                timeout--;
            }
            /* �ȴ��ɿ�Tamper���� */
            while (0 == gd_eval_key_state_get(KEY_TAMPER))
                ;
        }
        /* ����Ƿ���Wakeup���� */
        if (0 == gd_eval_key_state_get(KEY_WAKEUP)) // �������Wakeup����
        {
            transmit_message.tx_data[0] = 0xAA;            // ���÷������ݵ�һ���ֽ�Ϊ0xAA
            transmit_message.tx_data[1] = 0x55;            // ���÷������ݵڶ����ֽ�Ϊ0x55
            printf("\r\n can1 transmit data:");            // ��ӡ��ʾ��Ϣ
            for (i = 0; i < transmit_message.tx_dlen; i++) // ѭ����ӡ�������ݵ�ÿһ���ֽ�
            {
                printf(" %02x", transmit_message.tx_data[i]);
            }
            /* ������Ϣ */
            transmit_mailbox = can_message_transmit(CAN1, &transmit_message);
            /* �ȴ�������� */
            timeout = 0xFFFF;
            while ((CAN_TRANSMIT_OK != can_transmit_states(CAN1, transmit_mailbox)) && (0 != timeout))
            {
                timeout--;
            }
            /* �ȴ��ɿ�Wakeup���� */
            while (0 == gd_eval_key_state_get(KEY_WAKEUP))
                ;
        }
        /* ���CAN0��ȷ���յ����ݣ���ӡ���յ������� */
        if (SET == can0_receive_flag)
        {
            can0_receive_flag = RESET;
            printf("\r\n can0 receive data:");
            for (i = 0; i < receive_message.rx_dlen; i++)
            {
                printf(" %02x", receive_message.rx_data[i]);
            }
            gd_eval_led_toggle(LED4); // �����л�LED4��״̬
        }
        /* ���CAN1��ȷ���յ����ݣ���ӡ���յ������� */
        if (SET == can1_receive_flag)
        {
            can1_receive_flag = RESET;         // ������ձ�־λ
            gd_eval_led_toggle(LED5);          // ����LED5����˸
            printf("\r\n can1 receive data:"); // ��ӡCAN1�������ݵ���ʾ��Ϣ
            for (i = 0; i < receive_message.rx_dlen; i++)
            {
                printf(" %02x", receive_message.rx_data[i]); // ����ֽڴ�ӡCAN1���յ�������
            }
        }
        /* ���CAN0ͨ�ŷ����������ӡ������Ϣ */
        if (SET == can0_error_flag)
        {
            can0_error_flag = RESET;                 // ���CAN0�����־λ
            printf("\r\n can0 communication error"); // ��ӡCAN0ͨ�Ŵ�����Ϣ
        }
        /* ���CAN1ͨ�ŷ����������ӡ������Ϣ */
        if (SET == can1_error_flag)
        {
            can1_error_flag = RESET;                 // ���CAN1�����־λ
            printf("\r\n can1 communication error"); // ��ӡCAN1ͨ�Ŵ�����Ϣ
        }
    }
}

/*
 * ��������can_config
 * ��������������CAN����
 * �����������
 * �����������
 */
void can_config()
{
    can_parameter_struct can_parameter;     // CAN�����ṹ��
    can_filter_parameter_struct can_filter; // CAN�˲��������ṹ��

    can_struct_para_init(CAN_INIT_STRUCT, &can_parameter); // ��ʼ��CAN�����ṹ��
    can_struct_para_init(CAN_FILTER_STRUCT, &can_filter);  // ��ʼ��CAN�˲��������ṹ��

    /* ��ʼ��CAN�Ĵ��� */
    can_deinit(CAN0);
    can_deinit(CAN1);

    /* ����CAN���� */
    can_parameter.time_triggered = DISABLE;           // ��ʱ�䴥��ģʽ
    can_parameter.auto_bus_off_recovery = ENABLE;     // �Զ����߹رջָ�
    can_parameter.auto_wake_up = DISABLE;             // ���Զ�����
    can_parameter.auto_retrans = ENABLE;              // �Զ��ش�
    can_parameter.rec_fifo_overwrite = DISABLE;       // ����FIFO�����
    can_parameter.trans_fifo_order = DISABLE;         // ����FIFO����˳��
    can_parameter.working_mode = CAN_NORMAL_MODE;     // ��������ģʽ
    can_parameter.resync_jump_width = CAN_BT_SJW_1TQ; // ����ͬ����ת���
    can_parameter.time_segment_1 = CAN_BT_BS1_7TQ;    // ʱ���1
    can_parameter.time_segment_2 = CAN_BT_BS2_2TQ;    // ʱ���2

    /* ���ò����� */
#if CAN_BAUDRATE == 1000
    can_parameter.prescaler = 6; // 1MBps
#elif CAN_BAUDRATE == 500
    can_parameter.prescaler = 12; // 500KBps
#elif CAN_BAUDRATE == 250
    can_parameter.prescaler = 24; // 250KBps
#elif CAN_BAUDRATE == 125
    can_parameter.prescaler = 48; // 125KBps
#elif CAN_BAUDRATE == 100
    can_parameter.prescaler = 60; // 100KBps
#elif CAN_BAUDRATE == 50
    can_parameter.prescaler = 120; // 50KBps
#elif CAN_BAUDRATE == 20
    can_parameter.prescaler = 300; // 20KBps
#else
#error "please select list can baudrate in private defines in main.c "
#endif

    /* ��ʼ��CAN */
    can_init(CAN0, &can_parameter);
    can_init(CAN1, &can_parameter);

    /* ����CAN���������� */
    can_filter.filter_number = 0;                  // ���ù�������ţ������ǹ�����0
    can_filter.filter_mode = CAN_FILTERMODE_MASK;  // ���ù�����ģʽΪ����ģʽ
    can_filter.filter_bits = CAN_FILTERBITS_32BIT; // ���ù�����λ��Ϊ32λ
    can_filter.filter_list_high = 0x0000;          // ��16λ�������б�����Ϊ0
    can_filter.filter_list_low = 0x0000;           // ��16λ�������б�����Ϊ0
    can_filter.filter_mask_high = 0x0000;          // ��16λ��������������Ϊ0
    can_filter.filter_mask_low = 0x0000;           // ��16λ��������������Ϊ0
    can_filter.filter_fifo_number = CAN_FIFO0;     // ���ù�������FIFOΪFIFO0
    can_filter.filter_enable = ENABLE;             // ʹ�ܹ�����

    can_filter_init(&can_filter); // ��ʼ��CAN������

    /* CAN1��������� */
    can_filter.filter_number = 15; // ����CAN1���������Ϊ15
    can_filter_init(&can_filter);  // ��ʼ��CAN1������
}

/*!
    \brief      configure the nested vectored interrupt controller
    \param[in]  none
    \param[out] none
    \retval     none
*/
void nvic_config(void)
{
    /* configure CAN0 NVIC */
    nvic_irq_enable(CAN0_RX0_IRQn,0,0);

    /* configure CAN1 NVIC */
    nvic_irq_enable(CAN1_RX0_IRQn,1,1);
}

/*!
    \brief      configure the leds
    \param[in]  none
    \param[out] none
    \retval     none
*/
void led_config(void)
{
    gd_eval_led_init(LED2);
    gd_eval_led_init(LED3);
    gd_eval_led_init(LED4);
    gd_eval_led_init(LED5);
}

/**

@brief CANģ���GPIO�������ú���
**/
void can_gpio_config(void)
{
    /*ʹ��CANģ��ʱ��*/
    rcu_periph_clock_enable(RCU_CAN0);  // ʹ��CAN0����ʱ�ӡ�
    rcu_periph_clock_enable(RCU_CAN1);  // ʹ��CAN1����ʱ�ӡ�
    rcu_periph_clock_enable(RCU_GPIOB); // ʹ��GPIOB�˿�ʱ�ӡ�
    rcu_periph_clock_enable(RCU_GPIOD); // ʹ��GPIOD�˿�ʱ�ӡ�
    rcu_periph_clock_enable(RCU_AF);    // ʹ�ܸ��ù���ģ���ʱ�ӡ�

    /* ����CAN0��GPIO���� */
    gpio_init(GPIOD, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, GPIO_PIN_0);   // ����PD0Ϊ��������ģʽ
    gpio_init(GPIOD, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1); // ����PD1Ϊ���ù����������ģʽ
    gpio_pin_remap_config(GPIO_CAN0_FULL_REMAP, ENABLE);              // ��ӳ��CAN0��GPIO����ΪPD0��PD1

    /* ����CAN1��GPIO���� */
    gpio_init(GPIOB, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, GPIO_PIN_5);   // ����PB5Ϊ��������ģʽ
    gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_6); // ����PB6Ϊ���ù����������ģʽ
    gpio_pin_remap_config(GPIO_CAN1_REMAP, ENABLE);                   // ��ӳ��CAN1��GPIO����ΪPB5��PB6
}

/* retarget the C library printf function to the usart */
int fputc(int ch, FILE *f)
{
    usart_data_transmit(EVAL_COM0, (uint8_t)ch);
    while (RESET == usart_flag_get(EVAL_COM0, USART_FLAG_TBE));
    return ch;
}
