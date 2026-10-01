import esphome.codegen as cg
from esphome.components import i2c, sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
)

DEPENDENCIES = ["i2c"]

CONF_AMBIENT = "ambient"
CONF_OBJECT = "object"
CONF_EMISSIVITY = "emissivity"

mlx90632_ns = cg.esphome_ns.namespace("mlx90632")
MLX90632Component = mlx90632_ns.class_(
    "MLX90632Component", cg.PollingComponent, i2c.I2CDevice
)

_temp = dict(
    unit_of_measurement=UNIT_CELSIUS,
    accuracy_decimals=2,
    device_class=DEVICE_CLASS_TEMPERATURE,
    state_class=STATE_CLASS_MEASUREMENT,
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(MLX90632Component),
            cv.Optional(CONF_AMBIENT): sensor.sensor_schema(**_temp),
            cv.Optional(CONF_OBJECT): sensor.sensor_schema(**_temp),
            # Leaves are ~0.95-0.98. 1.0 = no correction.
            cv.Optional(CONF_EMISSIVITY, default=0.95): cv.float_range(min=0.1, max=1.0),
        }
    )
    .extend(cv.polling_component_schema("10s"))
    .extend(i2c.i2c_device_schema(0x3A))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    cg.add(var.set_emissivity(config[CONF_EMISSIVITY]))
    if CONF_AMBIENT in config:
        cg.add(var.set_ambient_sensor(await sensor.new_sensor(config[CONF_AMBIENT])))
    if CONF_OBJECT in config:
        cg.add(var.set_object_sensor(await sensor.new_sensor(config[CONF_OBJECT])))
