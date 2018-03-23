#include <qtinterfacetypes.h>
#include <QFileInfo>
#include <QTextStream>
#include <QDir>
#include <QRegularExpression>
#include <QColor>
#include <QSize>
#include "Framework/Tools.hpp"
#include "mainwindow.h"

RegisterModel::RegisterModel(QString name, QString value) {
  this->name = name;
  this->value = value;
}

QString RegisterModel::getName() const {
  return name;
}

QString RegisterModel::getValue() const {
  return value;
}

void RegisterModelList::clear() {
  beginRemoveRows(QModelIndex(), 0, rowCount());
  /*while(!registers.isEmpty()) {
    registers.takeLast();
  }*/
  registers.clear();
  endRemoveRows();
}

RegisterModelList::RegisterModelList(QObject *parent)
  : QAbstractListModel(parent)
{ }

QHash<int, QByteArray> RegisterModelList::roleNames() const
{
  QHash<int, QByteArray> roles;
  roles[NameRole] = "name";
  roles[ValueRole] = "value";
  return roles;
}

void RegisterModelList::addRegister(const RegisterModel &reg) {
  beginInsertRows(QModelIndex(), rowCount(), rowCount());
  registers.push_back(reg);
  endInsertRows();
}

void RegisterModelList::update(QList<RegisterModel> &newList) {
  bool reset = false;
  if(newList.size()!=registers.size())
    reset = true;
  else {
    for(int i=0;i<newList.size();i++)
      if(newList.at(i).getName()!=registers.at(i).getName()) {
        reset = true;
        break;
      }
  }
  if(reset) {
    beginResetModel();
    registers.clear();
    registers.append(newList);
    endResetModel();
  } else {
    for(int i=0;i<newList.size();i++)
      if(newList.at(i).getValue()!=registers.at(i).getValue()) {
        registers.removeAt(i);
        registers.insert(i,newList.at(i));
        dataChanged(index(i), index(i));
      }
  }
}

int RegisterModelList::rowCount(const QModelIndex & parent) const {
  return registers.count();
}

RegisterModel RegisterModelList::getByIndex(int index) {
  return registers[index];
}

QVariant RegisterModelList::data(const QModelIndex & index, int role) const {
  if (index.row() < 0 || index.row() > registers.count())
      return QVariant();

  const RegisterModel &reg = registers[index.row()];
  /*if (role == NameRole)
      return reg.getName();
  else if (role == ValueRole)
      return reg.getValue();*/
  if (role == Qt::DisplayRole) {
    return QString("%1 =  %2")
    .arg(reg.getName(), -4)
    .arg(reg.getValue());
  }
  return QVariant();
}

void RegisterModelList::onRegisterChanged(std::string name, std::string value) {
  for(int i=0;i<registers.size();i++)
    if(QString::fromStdString(name) == registers.at(i).getName()) {
      registers.removeAt(i);
      registers.insert(i,RegisterModel(QString::fromStdString(name), QString("%1").arg(QString::fromStdString(value),name=="SR"?4:8,QChar('0'))));
      dataChanged(index(i), index(i));
    }
}

MemoryModelList::MemoryModelList(MainWindow* window, QObject *parent)
  : QAbstractListModel(parent),
  window(window) {
  connect(window,SIGNAL(memoryChanged()),this,SLOT(memoryChanged()));
}

int MemoryModelList::rowCount(const QModelIndex & parent) const {
  if(window->interface==NULL)
    return 0;
  return window->interface->GetMaximumAddress(0)/16;
}

int MemoryModelList::columnCount(const QModelIndex & parent) const {
  return 2;
}

QVariant MemoryModelList::headerData(int section, Qt::Orientation orientation, int role) const {
  if(role == Qt::DisplayRole) {
    if(orientation==Qt::Horizontal) {
      switch(section) {
        /*case 0:
          return QString("Address");*/
        case 0:
          return QString("Values");
        case 1:
          return QString("ASCII");
        default:
          return QVariant();
      }
    } else if (orientation==Qt::Vertical) {
    return QString::fromStdString(IntToString(section*16,8));
    }
  }
  return QVariant();
}

void MemoryModelList::memoryChanged() {
  dataChanged(index(0,0),index(rowCount(),columnCount()));
}

void MemoryModelList::memoryReset() {
  beginResetModel();
  endResetModel();
}

QHash<int, QByteArray> MemoryModelList::roleNames() const
{
   QHash<int, QByteArray> roles;
  roles[ValuesRole] = "Values";
  roles[AsciiRole] = "ASCII";
  return roles;
}

QVariant MemoryModelList::data(const QModelIndex & index, int role) const {
  if (index.row() < 0 || index.row() > rowCount())
    return QVariant();
  if (role == Qt::DisplayRole) {
    if(window==NULL || window->interface==NULL)
      return QVariant();
    if(index.column()==0)
      return QString::fromStdString(window->interface->ListMemory(0, IntToString(16*index.row(),8), 16, 17)).toUpper();
    else if(index.column()==1) {
      QString vals = QString::fromStdString(window->interface->ListMemory(0, IntToString(16*index.row(),8), 16, 17));
      vals = vals.replace("\n","");
      QStringList list = vals.split(' ');
      vals.clear();
      bool ok;
      int v;
      for(int i=0;i<16;i++) {
        v = ((QString) list.at(i)).toInt(&ok, 16);
        if(ok)
          vals += QString("%1").arg(static_cast<char>(v));
        else
          vals += ".";
      }
      return vals;
    }
  } else if(role == Qt::FontRole) {
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    return font;
  }
  return QVariant();
}

void MemoryModelList::updateRowCount() {
  //Get memory max address
}


void BreakpointModelList::clear() {
  beginRemoveRows(QModelIndex(), 0, rowCount());
  /*while(!addresses.isEmpty()) {
    addresses.takeLast();
  }*/
  addresses.clear();
  endRemoveRows();
}

BreakpointModelList::BreakpointModelList(QObject *parent)
  : QAbstractListModel(parent)
{ }

bool BreakpointModelList::addBreakpoint(const Address &address) {
  int i;
  for(i=0;i<addresses.length();i++) {
    if(addresses[i]==address)
      return false;
    if(addresses[i]>address)
      break;
  }
  beginInsertRows(QModelIndex(), rowCount(), rowCount());
  if(i==addresses.length())
    addresses.push_back(address);
  else
    addresses.insert(i, address);
  endInsertRows();
  emit breakpointAdded(address);
  return true;
}

bool BreakpointModelList::removeBreakpoint(const Address &address) {
  int i = addresses.indexOf(address);
  if(i==-1)
    return false;
  beginRemoveRows(QModelIndex(), i, i);
  addresses.removeAt(i);
  endInsertRows();
  emit breakpointRemoved(address);
}

bool BreakpointModelList::contains(const Address &address) {
  return addresses.contains(address);
}

int BreakpointModelList::rowCount(const QModelIndex & parent) const {
  return addresses.count();
}

Address BreakpointModelList::getByIndex(int index) {
  return addresses[index];
}

QVariant BreakpointModelList::data(const QModelIndex & index, int role) const {
  if (index.row() < 0 || index.row() > addresses.count())
    return QVariant();

  const Address &address = addresses[index.row()];
  if (role == Qt::DisplayRole) {
    return QString("%1").arg(address, 8, 16, QChar('0')).toUpper();
  }
  return QVariant();
}

CodeLine::CodeLine(Address address, int lineNum, QString line) :
  address(address), lineNum(lineNum), content(line) { }

QString CodeLine::getContent() const {
  return content;
}

Address CodeLine::getAddress() const {
  return address;
}

int CodeLine::getLineNum() const {
  return lineNum;
}

CodeLineList::CodeLineList(BreakpointModelList *breakpoints = NULL, QObject *parent):
  breakpoints(breakpoints) {
  if(breakpoints!=NULL) {
    connect(breakpoints, SIGNAL(breakpointAdded(Address)), this, SLOT(onBreakpointAdded(Address)));
    connect(breakpoints, SIGNAL(breakpointRemoved(Address)), this, SLOT(onBreakpointRemoved(Address)));
  }
}

int CodeLineList::rowCount(const QModelIndex & parent) const {
  return lines.count();
}

QVariant CodeLineList::data(const QModelIndex & index, int role) const {
  if (index.row() < 0 || index.row() > lines.count())
    return QVariant();

  const CodeLine &line = lines[index.row()];
  if (role == Qt::DisplayRole) {
    //return line.getContent();
    if(index.row()==0 || lines[index.row()-1].getAddress()!=line.getAddress())
      return QString("%1 %2").arg(QString::fromStdString(IntToString(line.getAddress(),8))).arg(line.getContent());
    else
      return QString("         %1").arg(line.getContent());
  } else if(role == Qt::ForegroundRole) {
    if(breakpoints!=NULL && breakpoints->contains(line.getAddress())) {
      //if(index.row()==lines.size()-1 || lines[index.row()+1].getAddress()!=line.getAddress())
        return QVariant(QColor(255,0,0));
    }
  } else if(role == Qt::BackgroundRole) {
    if(line.getAddress()==lastPC && (index.row()==lines.size()-1 || line.getAddress() != lines[index.row()+1].getAddress())) {
      //if(index.row()==lines.size()-1 || lines[index.row()+1].getAddress()!=line.getAddress())
        return QVariant(QColor(0,160,0,80));
    }
  }
  return QVariant();
}

void CodeLineList::clear() {
  beginRemoveRows(QModelIndex(), 0, rowCount());
  lines.clear();
  endRemoveRows();
}

void CodeLineList::loadListing(const std::string &fileName) {
  clear();
  QString ext[] = {".lis", ".Lis", ".l", ".list", ".L", ".List", ".LIS", ".LIST", ".lst", ".Lst", ".LST", ".Listing", ".listing"};
  int len = sizeof(ext)/sizeof(ext[0]);
  QFileInfo fileBase(QString::fromStdString(fileName));
  fileBase.setFile(fileBase.path() + QDir::separator() + fileBase.completeBaseName());
  QFileInfo listingFile;
  bool found = false;
  for (int i=0;i<len && !found;i++) {
    listingFile.setFile(fileBase.absoluteFilePath()+ext[i]);
    if(listingFile.exists()) {
      found = true;
    }
  }
  if(!found)
    return;
  QFile file(listingFile.absoluteFilePath());
  if(!file.open(QIODevice::ReadOnly)) {
      return;
  }
  QTextStream in(&file);
  QRegularExpression regex("([0-9a-fA-F]+)  ([^ ].*?  +| *)([0-9]+)  (.*)");
  QRegularExpressionMatch match;
  beginResetModel();
  while(!in.atEnd()) {
    QString line = in.readLine();
    bool valid = regex.isValid();
    if((match = regex.match(line)).hasMatch()) {
      Address address = StringToInt(((QString) match.captured(1)).toUtf8().data());
      int lineNum = ((QString) match.captured(3)).toInt();
      lines.append(CodeLine(address, lineNum, match.captured(4)));
    }
  }
  file.close();
  endResetModel();
  emit listingLoaded();
}

const CodeLine* CodeLineList::lineAt(int index) {
    return index<lines.size()?&lines.at(index):NULL;
}

void CodeLineList::onBreakpointAdded(const Address address) {
  for(int i=lines.length()-1;i>=0&&lines.at(i).getAddress()>=address;i--) {
    if(lines.at(i).getAddress()==address) {
      dataChanged(index(i),index(i));
      //break;
    }
  }
}

void CodeLineList::onBreakpointRemoved(const Address address) {
  for(int i=lines.length()-1;i>=0&&lines.at(i).getAddress()>=address;i--) {
    if(lines.at(i).getAddress()==address) {
      dataChanged(index(i),index(i));
      //break;
    }
  }
}

void CodeLineList::onProgramCounterChanged(Address address) {
  Address aux = lastPC;
  lastPC = address;
  for(int i=lines.length()-1;i>=0&&(lines.at(i).getAddress()>=aux || lines.at(i).getAddress()>=lastPC);i--) {
    if(lines.at(i).getAddress()==address) {
      dataChanged(index(i),index(i));
    }
    if(lines.at(i).getAddress()==lastPC) {
      lastPCIndex = index(i);
      dataChanged(index(i),index(i));
    }
  }
}

QModelIndex CodeLineList::getLastPCIndex() {
  return lastPCIndex;
}

QModelIndex CodeLineList::getIndexFromAddress(Address address) {
  for(int i=lines.length()-1;i>=0&&lines.at(i).getAddress()>=address;i--) {
    if(lines.at(i).getAddress()==address) {
      return index(i);
    }
  }
  return index(-1,0);
}
