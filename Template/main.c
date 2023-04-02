#include "gd32f30x.h"
#include <stdio.h>
#include "main.h"
#include "gd32f30x_gpio.h"

/* ��ɫ */
#define ROLE (e_master)
// #define ROLE (e_slave_0)
/* CAN���Ͳ��� */
can_trasnmit_message_struct transmit_message;
/* CAN���ղ��� */
can_receive_message_struct receive_message;
/* CAN�˲������� */
can_filter_parameter_struct can_filter;
/* ͨ��ID */
COMMUNICATION_ID_t communication_id[SLAVE_COUNT];

/* ȷ��֡��ʽ */
const uint8_t sack_frame [8] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
/* ����֡��ʽ */
const uint8_t data_frame [8] = {0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11};
/* �ӽڵ���� */
uint8_t g_slave_index = 0;
/* ���ڵ�ظ�ȷ��֡��־ */
uint8_t g_master_ack_flag = RESET;

int main(void)
{
    /* ����GPIO���� */
    can_gpio_config();

    /* ��ʼ��can���ղ��� */
    can_struct_para_init(CAN_RX_MESSAGE_STRUCT, &receive_message);

    /* ��ʼ��can���Ͳ���  */
    can_struct_para_init(CAN_TX_MESSAGE_STRUCT, &transmit_message);

    /* �����ж������� */
    nvic_config();

    /* ��ʼ������0 */
    uart0_init();

    /* ��ʼ��LED */
    led_init();

    /* ��ʼ��CAN�͹����� */
    can_config();
    
    /* ��ʼ��ͨ��id */
    init_communication_id();

    if (ROLE == e_master)
    {
        /* ���ö�ʱ��3 */
        timer3_init(5000); // ��ʱ��3ÿ5000��5�룩���������ڲ���һ���жϽ������ڵ����ڷ�������
    }
    else
    {
        slave_task(ROLE);
    }

    while (1)
    {

    }
}

/**

@brief ������������ָ���ӽڵ㷢�����ݲ�����CAN������

@param[in] slave_index ָ���ӽڵ������

@note ���ݷ��ͱ�ʶ��Ϊָ���ӽڵ�����ձ�ʶ������������Ϊ�̶���11 11 11 11 11 11 11 11

@note ���ݷ��ͺ���յ��ӽڵ��ȷ��֡�����ڵ���Ҫ�ظ�ȷ��֡

@note ȷ��֡�������ݸ�ʽ�̶�Ϊ��FF FF FF FF FF FF FF FF�����ȹ̶�Ϊ8���ֽ�

@note ���ڵ�����������ʱ��˸��ɫLED����������ʱ��˸��ɫLED

@note ���ڵ㷢�����ݺ������CAN�������Խ��մӽڵ��ȷ��֡

@note ���ڵ�ᰴ�մӽڵ���������η������ݣ��������ﵽ���ֵʱ���´�0��ʼѭ��
*/
void master_task(const uint8_t slave_index)
{
    // ��ָ���ӽڵ㷢������
    can_send_data(SEND_DATA, communication_id[slave_index].master_tx_id);

    // ����CAN�������Խ��մӽڵ��ȷ��֡
    config_can0_filter(communication_id[slave_index].master_rx_id);

    // ������һ���ӽڵ������
    g_slave_index = (slave_index + 1) % SLAVE_COUNT;
}

void slave_task(const uint8_t slave_seq)
{
    config_slave_communication_id(slave_seq);
}

void config_slave_communication_id(const uint8_t slave_seq)
{
    // ����CAN�������Խ������ڵ��ȷ��֡
    if (slave_seq < SLAVE_COUNT)
    {
        config_can0_filter(communication_id[slave_seq].slave_rx_id);
    }
}


void can_send_data(const uint8_t send_data, const uint32_t tx_sfid)
{
    uint32_t timeout = 0xFFFF;    // ��ʼ�����ͳ�ʱʱ��Ϊ0xFFFF
    uint8_t transmit_mailbox = 0; // ���巢��������Ϊ0

    transmit_message.tx_sfid = tx_sfid;                                            // ��������Ϣ�ı�׼֡ID����Ϊtx_sfid
    memset(transmit_message.tx_data, send_data, sizeof(transmit_message.tx_data)); // ���������ݵ���������Ϊsend_data

    transmit_mailbox = can_message_transmit(CAN0, &transmit_message); // ����CAN��Ϣ

    while (can_transmit_states(CAN0, transmit_mailbox) != CAN_TRANSMIT_OK && timeout--)
        ; // �ȴ��������

    led_twinkle(e_red_led);

    printf("\r\ntx [%02x]: ",transmit_message.tx_sfid);
    for (uint8_t i = 0; i < transmit_message.tx_dlen; i++) // ������������
    {
        printf("%02x ", transmit_message.tx_data[i]); // ��ӡÿһ������
    }
    printf("\r\n"); // ����
}

void ckeck_receive_data(void)
{
    if (ROLE == e_master)
    {
        if ((communication_id[g_slave_index].master_rx_id == receive_message.rx_sfid) &&
            (CAN_FF_STANDARD == receive_message.rx_ff) &&
            (8 == receive_message.rx_dlen) &&
            (0 == memcmp(receive_message.rx_data, sack_frame, sizeof(sack_frame))))
        {
            printf("\r\ncheck receive data success , start send sack frame to slave !\r\n");
            can_send_data(0xff, communication_id[g_slave_index].master_tx_id); // �ظ�ȷ��֡
        }
        else
        {
            printf("\r\ncheck receive data fail !\r\n");
        }
    }
    else if (ROLE < SLAVE_COUNT)
    {
        // �Ϸ�����
        if ((receive_message.rx_sfid == communication_id[ROLE].slave_rx_id) &&
            (CAN_FF_STANDARD == receive_message.rx_ff) &&
            (8 == receive_message.rx_dlen))

        {
            if (0 == memcmp(receive_message.rx_data, data_frame, sizeof(data_frame))) // ����֡
            {
                printf("\r\ncheck receive data success , start send sack frame to master !\r\n");
                can_send_data(0xff, communication_id[ROLE].slave_tx_id); // �ظ�ȷ��֡
                timer3_init(5);                                          // ����5���볬ʱ���
                led_off(e_red_led);
            }
            else if (0 == memcmp(receive_message.rx_data, sack_frame, sizeof(sack_frame))) // ȷ��֡
            {
                can_send_data(0xff, communication_id[ROLE].slave_tx_id); // �ظ�ȷ��֡
                g_master_ack_flag = SET;
            }
            else
            {
                g_master_ack_flag = RESET;
            }
        }
        else
        {
            printf("\r\ncheck receive data fail !\r\n");
        }
    }
}
/*
 * ��������can_config
 * ��������������CAN����
 * �����������
 * �����������
 */
void can_config(void)
{
    can_parameter_struct can_parameter;     // CAN�����ṹ��   

    can_struct_para_init(CAN_INIT_STRUCT, &can_parameter); // ��ʼ��CAN�����ṹ��

    /* ��ʼ��CAN�Ĵ��� */
    can_deinit(CAN0);

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

    init_can0_filter(); 

    can_interrupt_enable(CAN0, CAN_INT_RFNE0); // ʹ��CAN0�Ľ���FIFO0�ǿ��ж�

    printf("\r\nconfigure can success !\r\n");
}

void init_can0_filter(void)
{
    can_struct_para_init(CAN_FILTER_STRUCT, &can_filter); // ��ʼ��CAN�˲��������ṹ��
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

    printf("\r\n init can0 filter success !\r\n");
}

void config_can0_filter(uint16_t filter_id)
{
    can_filter.filter_list_high = 0x0000;   // �������б��λΪ��
    can_filter.filter_list_low = filter_id; // �������б��λΪ0x01
    can_filter.filter_mask_high = 0x0000;   // �����λΪ0
    can_filter.filter_mask_low = 0xFF00;    // �����λΪ0xFF00����ʾֻƥ��ID��8λ

    printf("\r\n config filter id : %02x  !\r\n", can_filter.filter_list_low);
}
/*
!
    \brief      configure the nested vectored interrupt controller
    \param[in]  none
    \param[out] none
    \retval     none
*/
void nvic_config(void)
{
    /* ����CAN0 NVIC */
    nvic_irq_enable(USBD_LP_CAN0_RX0_IRQn, 0, 0);
    printf("\r\nCAN0 NVIC config success !\r\n");

    /* ���ö�ʱ��3 NVIC */
    nvic_irq_enable(TIMER3_IRQn, 1, 0);
    printf("\r\ntimer3 NVIC config success !\r\n");
}

void led_on(LED_e led)
{
    if (led == e_red_led)
    {
        GPIO_BOP(GPIOB) = GPIO_PIN_10;
    }
    else if (led == e_green_led)
    {
        GPIO_BOP(GPIOB) = GPIO_PIN_11;
    }
}

void led_off(LED_e led)
{
    if (led == e_red_led)
    {
        GPIO_BC(GPIOB) = GPIO_PIN_10;
    }
    else if (led == e_green_led)
    {
        GPIO_BC(GPIOB) = GPIO_PIN_11;
    }
}

void led_twinkle(LED_e led)
{
    for (uint8_t i = 0; i < 3; i++)
    {
        led_on(led);
        accurate_delay_ms(200);
        led_off(led);
    }
}

void led_init(void)
{

    rcu_periph_clock_enable(RCU_GPIOB);
    /* ��ʼ����ɫled*/
    gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10);
    led_off(e_green_led);

    /* ��ʼ����ɫled*/
    gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_11);
    led_off(e_red_led);

    printf("\r\nled init success !\r\n");
}

/**

@brief CANģ���GPIO�������ú���
**/
void can_gpio_config(void)
{
    /* ʹ��CAN0��GPIOBʱ�� */
    rcu_periph_clock_enable(RCU_CAN0);  
    rcu_periph_clock_enable(RCU_GPIOB); 

    /* ����PB8��PB9ΪCAN0��RX��TX���� */
    gpio_init(GPIOB, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, GPIO_PIN_8);  // RX
    gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9); // TX
}

/* ��ӳ���ӡ����������0 */
int fputc(int ch, FILE *f)
{
    usart_data_transmit(USART0, (uint8_t)ch);
    while (RESET == usart_flag_get(USART0, USART_FLAG_TBE))
        ;
    return ch;
}

/*!

@brief ��ʼ������0

@param none

@retval none
*/
void uart0_init(void)
{
    /* ʹ��GPIOʱ�� */
    rcu_periph_clock_enable(RCU_GPIOA);

    /* ʹ��USARTʱ�� */
    rcu_periph_clock_enable(RCU_USART0);

    /*��ʼ�����͹ܽ� */
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);

    /* ��ʼ�����չܽ� */
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_10);

    /* USART ���� */
    usart_deinit(USART0);                                 // �� USART0 ����ļĴ����ָ�����λֵ��ȷ�������������״̬��ʼ���á�
    usart_baudrate_set(USART0, 115200U);                  // ���� USART0 �Ĳ�����Ϊ 115200��
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);   // ʹ�� USART0 �Ľ��չ��ܡ�
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE); // ʹ�� USART0 �ķ��͹��ܡ�
    usart_enable(USART0);                                 // ʹ�� USART0 ���衣
}

/**

@brief �ú���ʹ���˶�ʱ��2��ʵ�ֺ��뼶�ӳ٣�ͬʱ����Ӱ��CAN0�жϡ������Ĳ���ms����ָ���ӳٵĺ�����������ӳ�ʱ��Ϊ6553.5����

@param ms: ��ʱ�ĺ�����

@retval None
**/
void accurate_delay_ms(uint16_t ms)
{
    rcu_periph_clock_enable(RCU_TIMER2); // ʹ�ܶ�ʱ�� 2 ʱ��

    timer_parameter_struct timer_initpara;
    timer_struct_para_init(&timer_initpara); // ʹ��Ĭ��ֵ��ʼ����ʱ������

    timer_initpara.prescaler = SystemCoreClock / 10000 - 1; // ��ʱ�� 2 ʱ��Ƶ��Ϊϵͳʱ��Ƶ�ʵ� 1/10000
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = ms * 10;     // ��ʱ�� 2 ������Ϊ��Ҫ��ʱ�ĺ��������� 10����Ϊʱ��Ƶ��Ϊ 1/10000��
    timer_init(TIMER2, &timer_initpara); // ��ʼ����ʱ�� 2

    timer_flag_clear(TIMER2, TIMER_FLAG_UP); // �����ʱ�� 2 �ļ�����־λ
    timer_enable(TIMER2);                    // ������ʱ�� 2

    while (!timer_flag_get(TIMER2, TIMER_FLAG_UP))
    {
    } // �ȴ���ʱ�� 2 ������ 0

    timer_disable(TIMER2);                // ֹͣ��ʱ�� 2
    rcu_periph_clock_disable(RCU_TIMER2); // �رն�ʱ�� 2 ʱ��
}

/*!
\brief CAN0�����жϺ���
\param[in] ��
\param[out] ��
\retval ��
*/

void USBD_LP_CAN0_RX0_IRQHandler(void)
{
    can_message_receive(CAN0, CAN_FIFO0, &receive_message); // ��ȡCAN0�Ľ��ջ���FIFO0�е���Ϣ���ṹ��receive_message��

    led_twinkle(e_green_led); // ��ɫָʾ����˸

    ckeck_receive_data();//�ж��Ƿ����ͨ��Э��

    printf("\r\nrx [%02x]: ", receive_message.rx_sfid);
    for (uint8_t i = 0; i < receive_message.rx_dlen; i++) // ������������
    {
        printf("%02x ", receive_message.rx_data[i]); // ��ӡÿһ������
    }
    printf("\r\n"); // ����
}

void init_communication_id(void)
{
    for (uint8_t i = 0; i < SLAVE_COUNT; i++)
    {
        communication_id[i].slave_tx_id = SLAVE_TX_ID_START + i;
        communication_id[i].slave_rx_id = SLAVE_RX_ID_START + i;
        communication_id[i].master_tx_id = MASTER_TX_ID_START + i;
        communication_id[i].master_rx_id = MASTER_RX_ID_START + i;
    }
}

void TIMER3_IRQHandler(void)
{
    if (timer_flag_get(TIMER3, TIMER_FLAG_UP) == SET)
    {
        if (ROLE == e_master) // ���ڵ�
        {
            timer_flag_clear(TIMER3, TIMER_FLAG_UP);
            master_task(g_slave_index); // ÿ5�����һ����������
        }
        else // �ӽڵ�
        {
            /* �ӽڵ㳬ʱ��� */
            if (g_master_ack_flag == RESET)
            {
                led_on(e_red_led); // ��ʱ���������
                printf("\r\n[%d]: wait master sack time out !\r\n", ROLE);
            }
        }
    }
}

void timer3_init(uint32_t period)
{
    /* ʹ�ܶ�ʱ��3ʱ�� */
    rcu_periph_clock_enable(RCU_TIMER3);

    /* ��ʱ��3�Ļ������� */
    timer_parameter_struct timer_initpara;
    timer_struct_para_init(&timer_initpara);
    timer_initpara.prescaler = SystemCoreClock / 10000 - 1;
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = period * 10;  // ÿperiod���봥��һ���ж�
    timer_init(TIMER3, &timer_initpara);

    /* ʹ�ܶ�ʱ��3�ж� */
    timer_interrupt_enable(TIMER3, TIMER_INT_UP);

    /* ������ʱ��3 */
    timer_enable(TIMER3);
}
