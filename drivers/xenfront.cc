#include <string.h>
#include <sstream>

#include "drivers/xenfront.hh"
#include <osv/debug.h>
#include <bsd/porting/bus.h>
#include <xen/xenstore/xenstorevar.h>
#include <xen/xenbus/xenbusb.h>

extern driver_t netfront_driver;
extern driver_t blkfront_driver;

namespace xenfront {

void xenfront_driver::otherend_changed(XenbusState state)
{
    if (_backend_changed)
        _backend_changed(&_bsd_dev, state);
}

void xenfront_driver::probe()
{
    if (_probe)
        _probe(&_bsd_dev);
}

int xenfront_driver::attach()
{
    if (_attach)
        return _attach(&_bsd_dev);
    return 0;
}

void xenfront_driver::set_ivars(struct xenbus_device_ivars *ivars)
{
    driver_t *table;
    std::stringstream ss;

    _otherend_path = std::string(ivars->xd_otherend_path);
    _node_path = std::string(ivars->xd_node);
    _type = std::string(ivars->xd_type);

    if (!strcmp(ivars->xd_type, "vif")) {
        table = &netfront_driver;
        _irq_type = INTR_TYPE_NET,
        ss << "xenfront-net";

    } else if (!strcmp(ivars->xd_type, "vbd")) {
        table = &blkfront_driver;
        _irq_type = INTR_TYPE_BIO;
        ss << "vblk";
    } else
        return;

    _driver_name = ss.str();
    device_method_t *dm = table->methods;
    for (auto i = 0; dm[i].id; i++) {
        if (dm[i].id == bus_device_probe)
            _probe = reinterpret_cast<xenfront::probe>(dm[i].func);
        if (dm[i].id == bus_device_attach)
            _attach = reinterpret_cast<xenfront::attach>(dm[i].func);
        if (dm[i].id == bus_xenbus_otherend_changed)
            _backend_changed = reinterpret_cast<xenfront::backend_changed>(dm[i].func);
    }
    _bsd_dev.softc = malloc(table->size);
    // Simpler and we don't expect driver loading to fail anyway
    assert(_bsd_dev.softc);
}

xenfront_driver::xenfront_driver()
{
}

xenfront_driver::~xenfront_driver()
{
}
}
