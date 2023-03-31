#include "gd32f30x.h"
#include <stdio.h>
#include "main.h"
#include "gd32f30x_gpio.h"

/* can���ձ�־λ */
FlagStatus can0_receive_flag;
/* can�����־λ */
FlagStatus can0_error_flag;
/* CAN���Ͳ�����ʼ�� */
static const can_trasnmit_message_struct transmit_message_init = {
    .tx_efid = 0x00,          // ��ʼ����չ֡ID
    .tx_ft = CAN_FT_DATA,     // ��ʼ����������֡����
    .tx_ff = CAN_FF_STANDARD, // ��ʼ����������֡��ʽΪ��׼֡
    .tx_dlen = 8,             // ��ʼ���������ݵĳ���Ϊ8
};

int main(void)
{
    // ��ʼ��CAN0
    __can_init__();

    // �����ж�������
    nvic_config();

    /* ��ʼ������0 */
    uart0_init();

    // ��ʼ��LED
    led_init();

    /* ��ʼ��CAN�͹����� */
    can_config();

    while (1)
    {
        if (ROLE == e_master)
        {
            can_send_data(0x11, 0x51);
            receive_data();
        }
    }
}

void __can_init__()
{
    can0_receive_flag = RESET; // ��CAN0���ձ�־����
    can0_error_flag = RESET;   // ��CAN0�����־����

    can_gpio_config(); // ����GPIO����
    printf("\r\ncan init success !\r\n");
}

void can_send_data(const uint8_t send_data, const uint32_t tx_sfid)
{
    uint32_t timeout = 0xFFFF;    // ��ʼ�����ͳ�ʱʱ��Ϊ0xFFFF
    uint8_t transmit_mailbox = 0; // ���巢��������Ϊ0

    can_trasnmit_message_struct transmit_message = transmit_message_init;          // ��������Ϣ�ĳ�ʼֵ����Ϊtransmit_message_init
    transmit_message.tx_sfid = tx_sfid;                                            // ��������Ϣ�ı�׼֡ID����Ϊtx_sfid
    memset(transmit_message.tx_data, send_data, sizeof(transmit_message.tx_data)); // ���������ݵ���������Ϊsend_data

    transmit_mailbox = can_message_transmit(CAN0, &transmit_message); // ����CAN��Ϣ

    while (can_transmit_states(CAN0, transmit_mailbox) != CAN_TRANSMIT_OK && timeout--)
        ; // �ȴ��������

    printf("\r\ntx: ");
    for (uint8_t i = 0; i < 8; i++) // ������������
    {
        printf("%02x ", transmit_message.tx_data[i]); // ��ӡÿһ������
    }
    printf("\r\n"); // ����
}

void receive_data()
{
    can_receive_message_struct receive_message;

    /* ���CAN0��ȷ���յ����ݣ���ӡ���յ������� */
    if (SET == can0_receive_flag)
    {
        can0_receive_flag = RESET;
        printf("\r\n can0 receive data:");
        for (uint8_t i = 0; i < receive_message.rx_dlen; i++)
        {
            printf(" %02x", receive_message.rx_data[i]);
        }
    }

    if (SET == can0_error_flag)
    {
        can0_error_flag = RESET;                 // ���CAN0�����־λ
        printf("\r\n can0 communication error"); // ��ӡCAN0ͨ�Ŵ�����Ϣ
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

    can_interrupt_enable(CAN0, CAN_INT_RFNE0); // ʹ��CAN0�Ľ���FIFO0�ǿ��ж�

    printf("\r\nconfigure can success !\r\n");
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
    nvic_irq_enable(CAN0_RX0_IRQn, 0, 0);
    printf("\r\nconfigure CAN0 NVIC success !\r\n");
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
    /*ʹ��CANģ��ʱ��*/
    rcu_periph_clock_enable(RCU_CAN0);  // ʹ��CAN0����ʱ�ӡ�
    rcu_periph_clock_enable(RCU_GPIOD); // ʹ��GPIOD�˿�ʱ�ӡ�
    rcu_periph_clock_enable(RCU_AF);    // ʹ�ܸ��ù���ģ���ʱ�ӡ�

    /* ����CAN0��GPIO���� */
    gpio_init(GPIOD, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, GPIO_PIN_0);   // ����PD0Ϊ��������ģʽ
    gpio_init(GPIOD, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1); // ����PD1Ϊ���ù����������ģʽ
    gpio_pin_remap_config(GPIO_CAN0_FULL_REMAP, ENABLE);              // ��ӳ��CAN0��GPIO����ΪPD0��PD1
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
void uart0_init()
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