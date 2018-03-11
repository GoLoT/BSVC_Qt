#ifndef QTINTERFACETYPES_H
#define QTINTERFACETYPES_H

#include <QAbstractListModel>
//#include "qtinterface.h"
#include "Framework/Types.hpp"
//#include "Framework/BreakpointList.hpp"
class MainWindow;
class RegisterModel {
public:
  RegisterModel(QString name, QString value);

  QString getName() const;
  QString getValue() const;

private:
  QString name;
  QString value;
};

class RegisterModelList : public QAbstractListModel {

  Q_OBJECT

public:
  enum RegisterRoles {
    NameRole = Qt::UserRole + 1,
    ValueRole
  };

  RegisterModelList(QObject *parent = 0);
  int rowCount(const QModelIndex & parent = QModelIndex()) const;
  QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
  void addRegister(const RegisterModel &reg);
  void clear();
  RegisterModel getByIndex(int index);
  bool changeRegister(const std::string name, const uint32_t value);
  void update(QList<RegisterModel> &newList);

protected slots:
  void onRegisterChanged(std::string name, std::string value);

protected:
  QHash<int, QByteArray> roleNames() const;

private:
  QList<RegisterModel> registers;

};

class DeviceModel {

};

class DeviceModelList : public QAbstractListModel {
  Q_OBJECT

};

class BreakpointModelList : public QAbstractListModel {
  Q_OBJECT

private:
  QList<Address> addresses;

signals:
  void breakpointAdded(const Address address);
  void breakpointRemoved(const Address address);

public:
  BreakpointModelList(QObject *parent = 0);
  int rowCount(const QModelIndex & parent = QModelIndex()) const;
  QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
  bool addBreakpoint(const Address &address);
  bool removeBreakpoint(const Address &address);
  void clear();
  Address getByIndex(int index);
  bool contains(const Address &address);
};

class MemoryModelList : public QAbstractListModel {
  Q_OBJECT

public:
  enum MemoryRoles {
    AddressRole = Qt::UserRole + 10,
    ValuesRole,
    AsciiRole
  };

  MemoryModelList(MainWindow* window, QObject *parent = 0);
  int rowCount(const QModelIndex & parent = QModelIndex()) const;
  int columnCount(const QModelIndex & parent = QModelIndex()) const;
  QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
  void updateRowCount();
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

protected:
  QHash<int, QByteArray> roleNames() const;

private:
  MainWindow* window;
  int rows;

public slots:
  void memoryChanged();
  void memoryReset();
};

class CodeLine {
public:
  CodeLine(Address address, int lineNum, QString line);

  QString getContent() const;
  Address getAddress() const;
  int getLineNum() const;

private:
  QString content;
  const int lineNum;
  const Address address;
};

class CodeLineList : public QAbstractListModel {
  Q_OBJECT

private:
  QList<CodeLine> lines;
  Address lastPC;
  QModelIndex lastPCIndex;
  BreakpointModelList* breakpoints;

public:
  CodeLineList(BreakpointModelList *breakpoints, QObject *parent = 0);
  int rowCount(const QModelIndex & parent = QModelIndex()) const;
  QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
  const CodeLine* lineAt(int index);
  QModelIndex getLastPCIndex();
  QModelIndex getIndexOf();
  QModelIndex getIndexFromAddress(Address address);

signals:
  void listingLoaded();

public slots:
  void clear(); //Call when new device created
  void loadListing(const std::string &fileName); //Call when program loaded
  void onBreakpointAdded(const Address address);
  void onBreakpointRemoved(const Address address);
  void onProgramCounterChanged(const Address address);
};

#endif // QTINTERFACETYPES_H
