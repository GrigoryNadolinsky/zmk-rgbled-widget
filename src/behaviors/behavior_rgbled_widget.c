#define DT_DRV_COMPAT zmk_behavior_rgbled_widget

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rgbled_widget, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/behavior.h>
#include <zmk_rgbled_widget/widget.h>

struct behavior_rgb_wdg_config {
    bool indicate_battery;
    bool indicate_connectivity;
    bool indicate_layer;
};

/* Инициализация поведения: при старте сразу показываем заряд только периферии */
static int behavior_rgb_wdg_init(const struct device *dev)
{
    const struct behavior_rgb_wdg_config *cfg = dev->config;
#if IS_ENABLED(CONFIG_RGBLED_WIDGET_BATTERY_SHOW_ONLY_PERIPHERALS)
    LOG_INF("Init: peripheral-only battery indication");
    indicate_battery();
#endif
    return 0;
}

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event)
{
#if IS_ENABLED(CONFIG_RGBLED_WIDGET)
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_rgb_wdg_config *cfg = dev->config;

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING)
    if (cfg->indicate_battery) {
        indicate_battery();
    }
#endif

    // --- ПОДСВЕТКА СТАТУСА BLE-СОЕДИНЕНИЯ ТОЛЬКО НА ПЕРИФЕРИЙНЫХ ПОЛОВИНАХ ---
    // Этот блок вызывает функцию indicate_connectivity() только в том случае,
    // если устройство НЕ является центральной частью (CONFIG_ZMK_SPLIT_ROLE_CENTRAL не определён).
    // Это позволяет отключить отображение статуса соединения на донгле при нажатии &ind_con,
    // сохраняя подсветку на периферийных половинках клавиатуры (left/right).
#if IS_ENABLED(CONFIG_ZMK_BLE)
#if !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    if (cfg->indicate_connectivity) {
        indicate_connectivity();
    }
#endif
#endif

    // --- ПОДСВЕТКА СТАТУСА BLE-СОЕДИНЕНИЯ НА ВСЕХ ЧАСТЯХ (ВКЛЮЧАЯ ДОНГЛ) ---
    // Этот блок вызывает функцию indicate_connectivity() на всех частях клавиатуры,
    // включая центральную часть (донгл). В результате при нажатии &ind_con
    // светодиод загорается как на периферийных половинках, так и на донгле.
    // Мы закомментировали его, чтобы отключить отображение BLE-состояния на донгле.
    /*
#if IS_ENABLED(CONFIG_ZMK_BLE)
    if (cfg->indicate_connectivity) {
        indicate_connectivity();
    }
#endif
    */

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    if (cfg->indicate_layer) {
        indicate_layer();
    }
#endif
#endif // IS_ENABLED(CONFIG_RGBLED_WIDGET)

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event)
{
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_rgb_wdg_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

#define RGBIND_INST(n)                                                                                 \
    static struct behavior_rgb_wdg_config behavior_rgb_wdg_config_##n = {                              \
        .indicate_battery     = DT_INST_PROP(n, indicate_battery),                                     \
        .indicate_connectivity = DT_INST_PROP(n, indicate_connectivity),                               \
        .indicate_layer       = DT_INST_PROP(n, indicate_layer),                                       \
    };                                                                                                 \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_rgb_wdg_init, NULL, NULL, &behavior_rgb_wdg_config_##n,        \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                          \
                            &behavior_rgb_wdg_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RGBIND_INST)
