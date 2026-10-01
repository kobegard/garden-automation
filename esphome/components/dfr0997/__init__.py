import esphome.codegen as cg
from esphome.components import i2c
import esphome.config_validation as cv
from esphome.const import CONF_COLOR, CONF_ID, CONF_LAMBDA, CONF_SIZE, CONF_X, CONF_Y

DEPENDENCIES = ["i2c"]
MULTI_CONF = True

CONF_LINES = "lines"
CONF_BACKGROUND_COLOR = "background_color"

dfr0997_ns = cg.esphome_ns.namespace("dfr0997")
DFR0997Component = dfr0997_ns.class_("DFR0997Component", cg.PollingComponent, i2c.I2CDevice)

SIZES = {"large": 0, "small": 1}  # firmware font: 0 = 24 px, 1 = 12 px
_rgb = cv.int_range(min=0, max=0xFFFFFF)

LINE_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_X): cv.int_range(min=0, max=319),
        cv.Required(CONF_Y): cv.int_range(min=0, max=239),
        cv.Optional(CONF_SIZE, default="large"): cv.enum(SIZES, lower=True),
        cv.Optional(CONF_COLOR, default=0xFFFFFF): _rgb,
        cv.Required(CONF_LAMBDA): cv.returning_lambda,
    }
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(DFR0997Component),
            cv.Optional(CONF_BACKGROUND_COLOR, default=0x000000): _rgb,
            cv.Required(CONF_LINES): cv.All(cv.ensure_list(LINE_SCHEMA), cv.Length(min=1, max=40)),
        }
    )
    .extend(cv.polling_component_schema("5s"))
    .extend(i2c.i2c_device_schema(0x2C))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    cg.add(var.set_background_color(config[CONF_BACKGROUND_COLOR]))
    for line in config[CONF_LINES]:
        fn = await cg.process_lambda(line[CONF_LAMBDA], [], return_type=cg.std_string)
        cg.add(var.add_line(line[CONF_X], line[CONF_Y], line[CONF_SIZE], line[CONF_COLOR], fn))
