#ifndef ITAGREPO_H
#define ITAGREPO_H
#include <QVector>
#include <QString>

struct TagInf;  // Forward declaration — full in host/domain/tag/TagInfo.h

class ITagRepo {
public:
    virtual ~ITagRepo() = default;
    virtual bool insert(const TagInf& tag) = 0;
    virtual bool update(const TagInf& tag) = 0;
    virtual bool remove(quint32 tagId) = 0;
    virtual TagInf findById(quint32 tagId) const = 0;
    virtual QVector<TagInf> findAll() const = 0;
    virtual bool loadFromJson(const QString& path) = 0;
    virtual bool saveToJson(const QString& path) const = 0;
};

#define ITagRepo_iid "com.dcsshell.ITagRepo"
#endif
