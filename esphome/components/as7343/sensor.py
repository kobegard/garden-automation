import esphome.codegen as cg
from esphome.components import i2c, sensor
import esphome.config_validation as cv
from esphome.const import CONF_ID, STATE_CLASS_MEASUREMENT

DEPENDENCIES = ["i2c"]

CONF_ATIME = "atime"
CONF_ASTEP = "astep"
CONF_PAR = "par"
CONF_PAR_FACTOR = "par_factor"
CONF_CLEAR = "clear"

# name -> index in the 18-channel auto-SMUX data block (Adafruit channel map)
CHANNELS = {
    "f1_405nm": 12,
    "f2_425nm": 6,
    "fz_450nm": 0,
    "f3_475nm": 7,
    "f4_515nm": 8,
    "f5_550nm": 15,
    "fy_555nm": 1,
    "fxl_600nm": 2,
    "f6_640nm": 9,
    "f7_690nm": 13,
    "f8_745nm": 14,
    "nir_855nm": 3,
}

as7343_ns = cg.esphome_ns.namespace("as7343")
AS7343Component = as7343_ns.class_("AS7343Component", cg.PollingComponent, i2c.I2CDevice)

_counts = sensor.sensor_schema(
    unit_of_measurement="cts",
    accuracy_decimals=2,
    state_class=STATE_CLASS_MEASUREMENT,
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(AS7343Component),
            # Integration time = (atime+1) * (astep+1) * 2.78 us; default ~50 ms.
            cv.Optional(CONF_ATIME, default=29): cv.int_range(min=0, max=255),
            cv.Optional(CONF_ASTEP, default=599): cv.int_range(min=0, max=65534),
            cv.Optional(CONF_CLEAR): _counts,
            # Sum of the 400-700 nm channels in basic counts x par_factor.
            # UNCALIBRATED until par_factor is fitted against a quantum meter.
            cv.Optional(CONF_PAR): sensor.sensor_schema(
                unit_of_measurement="µmol/m²/s",
                icon="mdi:white-balance-sunny",
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_PAR_FACTOR, default=1.0): cv.positive_float,
            **{cv.Optional(name): _counts for name in CHANNELS},
        }
    )
    .extend(cv.polling_component_schema("30s"))
    .extend(i2c.i2c_device_schema(0x39))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    cg.add(var.set_timing(config[CONF_ATIME], config[CONF_ASTEP]))
    cg.add(var.set_par_factor(config[CONF_PAR_FACTOR]))
    for name, index in CHANNELS.items():
        if name in config:
            cg.add(var.set_channel_sensor(index, await sensor.new_sensor(config[name])))
    if CONF_CLEAR in config:
        cg.add(var.set_clear_sensor(await sensor.new_sensor(config[CONF_CLEAR])))
    if CONF_PAR in config:
        cg.add(var.set_par_sensor(await sensor.new_sensor(config[CONF_PAR])))
