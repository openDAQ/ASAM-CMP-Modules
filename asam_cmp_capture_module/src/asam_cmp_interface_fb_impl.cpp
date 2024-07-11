#include <asam_cmp_capture_module/asam_cmp_interface_fb_impl.h>
#include <coreobjects/callable_info_factory.h>
#include <coreobjects/argument_info_factory.h>
#include <coretypes/listobject_factory.h>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

AsamCmpInterfaceFbImpl::AsamCmpInterfaceFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
{
    initProperties();
}

FunctionBlockTypePtr AsamCmpInterfaceFbImpl::CreateType()
{
    return FunctionBlockType("asam_cmp_interface", "AsamCmpInterface", "Asam CMP Interface");
}

void AsamCmpInterfaceFbImpl::initProperties()
{
    StringPtr propName = "InterfaceId";
    auto prop = IntPropertyBuilder(propName, 0).build();
    objPtr.addProperty(prop);

    propName = "PayloadType";
    ListPtr<StringPtr> payloadTypes{"UNDEFINED",
                                    "CAN",
                                    "CAN_FD",
                                    "LIN",
                                    "FLEXRAY",
                                    "DIGITAL",
                                    "UART_RS-232",
                                    "ANALOG",
                                    "ETHERNET",
                                    "SPI",
                                    "I2C",
                                    "GIGE_VISION",
                                    "MIPI_CSI-2_D-PHY"
    };
    prop = SelectionPropertyBuilder(propName, payloadTypes, 0).build();

    propName = "AddStream";
    prop = FunctionPropertyBuilder(propName, ProcedureInfo(List<IArgumentInfo>())).setReadOnly(true).build();
    objPtr.addProperty(prop);
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(
        propName, Procedure([this] { throw DaqException::exception("Not implemented"); }));

    propName = "RemoveStream";
    prop = FunctionPropertyBuilder(propName, ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("nInd", ctInt)))).setReadOnly(true).build();
    objPtr.addProperty(prop);
    objPtr.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(
        propName, Procedure([this](int nInd) { throw DaqException::exception("Not implemented"); }));
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
