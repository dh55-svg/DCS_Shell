#ifndef ICONFIGURABLE_H
#define ICONFIGURABLE_H
#include <QVariantMap>

class IConfigurable {
public:
    virtual ~IConfigurable() = default;
    virtual void configure(const QVariantMap& config) = 0;
};
#endif
