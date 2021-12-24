aml_library_sources_ifdef(
	CONFIG_GPIO
	gpio.c
	gpio_irq.c
	gpio_${SOC}_plat.c
)

