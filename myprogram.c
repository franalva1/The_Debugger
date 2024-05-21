#include "uart.h"
#include "debugger.h"
#include "timer.h"
#include "gpio.h"
#include "printf.h"

void main(void)
{
    uart_init();

 
    gpio_init();


   gpio_set_input(17);



   while(1) {


	printf("%d\n", gpio_read(17));

	timer_delay_ms(200);
   }



    
    uart_putchar(EOT);
}
