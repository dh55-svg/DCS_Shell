#ifndef NULLTAGREPO_H
#define NULLTAGREPO_H
#include "plugin_interface/ITagRepo.h"

class NullTagRepo : public ITagRepo {
public:
    bool insert(const TagInf&) override { return true; }
    bool update(const TagInf&) override { return true; }
    bool remove(quint32) override { return true; }
    TagInf findById(quint32) const override { return TagInf{}; }
    QVector<TagInf> findAll() const override { return m_tags; }
    bool loadFromJson(const QString&) override { return true; }
    bool saveToJson(const QString&) const override { return true; }
    void addTag(const TagInf& t) { m_tags.append(t); }
private:
    mutable QVector<TagInf> m_tags;
};
#endif
