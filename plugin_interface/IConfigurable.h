#ifndef ICONFIGURABLE_H
#define ICONFIGURABLE_H
#include <QObject>
#include <QVariantMap>

class IConfigurable {
public:
    virtual ~IConfigurable() = default;
    virtual void configure(const QVariantMap& config) = 0;
};

#define IConfigurable_iid "com.dcsshell.IConfigurable"
Q_DECLARE_INTERFACE(IConfigurable, IConfigurable_iid)
#endif
