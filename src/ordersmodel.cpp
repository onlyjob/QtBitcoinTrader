//  This file is part of Qt Bitcion Trader
//      https://github.com/JulyIGHOR/QtBitcoinTrader
//  Copyright (C) 2013-2014 July IGHOR <julyighor@gmail.com>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  In addition, as a special exception, the copyright holders give
//  permission to link the code of portions of this program with the
//  OpenSSL library under certain conditions as described in each
//  individual source file, and distribute linked combinations including
//  the two.
//
//  You must obey the GNU General Public License in all respects for all
//  of the code used other than OpenSSL. If you modify file(s) with this
//  exception, you may extend this exception to your version of the
//  file(s), but you are not obligated to do so. If you do not wish to do
//  so, delete this exception statement from your version. If you delete
//  this exception statement from all source files in the program, then
//  also delete it here.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "ordersmodel.h"
#include "main.h"
#include "exchange.h"

OrdersModel::OrdersModel()
	: QAbstractItemModel()
{
	checkDuplicatedOID=false;
	haveOrders=false;
	columnsCount=8;
	dateWidth=100;
	typeWidth=100;
	countWidth=20;
	statusWidth=100;

	textStatusList<<"CANCELED"<<"OPEN"<<"PENDING"<<"POST-PENDING"<<"INVALID";
}

OrdersModel::~OrdersModel()
{

}

int OrdersModel::rowCount(const QModelIndex &) const
{
	return oidList.count();
}

int OrdersModel::columnCount(const QModelIndex &) const
{
	return columnsCount;
}

void OrdersModel::clear()
{
	if(oidList.count()==0)return;

	beginResetModel();
	currentAsksPrices.clear();
	currentBidsPrices.clear();
	oidList.clear();
	dateList.clear();
	dateStrList.clear();
	typesList.clear();
	statusList.clear();
	amountList.clear();
	amountStrList.clear();
	priceList.clear();
	priceStrList.clear();
	totalList.clear();
	totalStrList.clear();
	symbolList.clear();

	haveOrders=false;
	if(checkDuplicatedOID)oidMapForCheckingDuplicates.clear();

	emit volumeAmountChanged(0.0, 0.0);
	endResetModel();
}

void OrdersModel::ordersChanged(QList<OrderItem> *orders)
{
	currentAsksPrices.clear();
	currentBidsPrices.clear();

	if(orders->count()==0)
	{
		delete orders;
		emit volumeAmountChanged(0.0,0.0);
		clear();
		return;
	}

	QHash<QByteArray,bool> existingOids;

	double volumeTotal=0.0;
	double amountTotal=0.0;

	for(int n=0;n<orders->count();n++)
	{
		bool isAsk=orders->at(n).type;
		QByteArray orderSymbol=orders->at(n).symbol;

		if(orderSymbol.startsWith(baseValues.currentPair.currAStr)&&isAsk)volumeTotal+=orders->at(n).amount-currentExchange->decAmountFromOpenOrder;
		if(orderSymbol.endsWith(baseValues.currentPair.currBStr)&&!isAsk)amountTotal+=(orders->at(n).amount-currentExchange->decAmountFromOpenOrder)*orders->at(n).price;

		if(orders->at(n).status>0&&orderSymbol==baseValues.currentPair.currSymbol)
		{
			if(isAsk)currentAsksPrices[orders->at(n).price]=true;
			else currentBidsPrices[orders->at(n).price]=true;
		}

		existingOids.insert(orders->at(n).oid,true);
		if(checkDuplicatedOID)(*orders)[n].date=oidMapForCheckingDuplicates.value(orders->at(n).oid,orders->at(n).date);

		int currentIndex=qLowerBound(dateList.begin(),dateList.end(),orders->at(n).date)-dateList.begin();
		int currentIndexUpper=qUpperBound(dateList.begin(),dateList.end(),orders->at(n).date)-dateList.begin();

		bool matchListRang=currentIndex>-1&&dateList.count()>currentIndex;

		if(matchListRang&&oidList.at(currentIndex)!=orders->at(n).oid)
			while(oidList.at(currentIndex)!=orders->at(n).oid&&currentIndex<currentIndexUpper)
			{
				if(currentIndex+1>-1&&dateList.count()>currentIndex+1)
					currentIndex++;
				else break;
			}

		if(matchListRang&&oidList.at(currentIndex)==orders->at(n).oid)
		{//Update
		 if(statusList.at(currentIndex)&&
			 (statusList.at(currentIndex)!=orders->at(n).status||
			  amountList.at(currentIndex)!=orders->at(n).amount||
			   priceList.at(currentIndex)!=orders->at(n).price))
			{
			statusList[currentIndex]=orders->at(n).status;

			amountList[currentIndex]=orders->at(n).amount;
			amountStrList[currentIndex]=orders->at(n).amountStr;

			priceList[currentIndex]=orders->at(n).price;
			priceStrList[currentIndex]=orders->at(n).priceStr;

			totalList[currentIndex]=orders->at(n).total;
			totalStrList[currentIndex]=orders->at(n).totalStr;
			}
		}
		else
		{//Insert
			beginInsertRows(QModelIndex(), currentIndex, currentIndex);

			oidList.insert(currentIndex,orders->at(n).oid);

			dateList.insert(currentIndex,orders->at(n).date);
			dateStrList.insert(currentIndex,orders->at(n).dateStr);

			typesList.insert(currentIndex,orders->at(n).type);
			statusList.insert(currentIndex,orders->at(n).status);

			amountList.insert(currentIndex,orders->at(n).amount);
			amountStrList.insert(currentIndex,orders->at(n).amountStr);

			priceList.insert(currentIndex,orders->at(n).price);
			priceStrList.insert(currentIndex,orders->at(n).priceStr);

			totalList.insert(currentIndex,orders->at(n).total);
			totalStrList.insert(currentIndex,orders->at(n).totalStr);

			symbolList.insert(currentIndex,orders->at(n).symbol);

			if(checkDuplicatedOID)oidMapForCheckingDuplicates.insert(orders->at(n).oid,orders->at(n).date);
			
			endInsertRows();
		}
	}

	for(int n=oidList.count()-1;n>=0;n--)//Removing Order
		if(existingOids.value(oidList.at(n),false)==false)
		{
			beginRemoveRows(QModelIndex(), n, n);
			if(checkDuplicatedOID)oidMapForCheckingDuplicates.remove(oidList.at(n));
			oidList.removeAt(n);
			dateList.removeAt(n);
			dateStrList.removeAt(n);
			typesList.removeAt(n);
			statusList.removeAt(n);
			amountList.removeAt(n);
			amountStrList.removeAt(n);
			priceList.removeAt(n);
			priceStrList.removeAt(n);
			totalList.removeAt(n);
			totalStrList.removeAt(n);
			symbolList.removeAt(n);
			endRemoveRows();
		}

	delete orders;

	if(haveOrders==false)
	{
		emit ordersIsAvailable();
		haveOrders=true;
	}
	countWidth=qMax(textFontWidth(QString::number(oidList.count()+1))+6,defaultHeightForRow);
	emit volumeAmountChanged(volumeTotal, amountTotal);
	emit dataChanged(index(0,0),index(oidList.count()-1,columnsCount-1));
}

QVariant OrdersModel::data(const QModelIndex &index, int role) const
{
	if(!index.isValid())return QVariant();
	int currentRow=oidList.count()-index.row()-1;
	if(currentRow<0||currentRow>=oidList.count())return QVariant();

	if(role==Qt::WhatsThisRole)return symbolList.at(currentRow);

	if(role==Qt::UserRole)
	{
		if(statusList.at(currentRow))return oidList.at(currentRow);
		return QVariant();
	}

	int indexColumn=index.column()-1;

	if(role==Qt::EditRole)
	{
		switch(indexColumn)
		{
		case -1: return oidList.count()-currentRow;
		case 0: return dateList.at(currentRow);
		case 1: return typesList.at(currentRow);
		case 2: return statusList.at(currentRow);
		case 3: return amountList.at(currentRow);
		case 4: return priceList.at(currentRow);
		case 5: return totalList.at(currentRow);
		case 6: return oidList.at(currentRow);
		}
		return oidList.count()-currentRow;
	}

	if(role==Qt::StatusTipRole)
	{
		QString copyText=dateStrList.at(currentRow)+"\t"+(typesList.at(currentRow)?textAsk:textBid)+"\t";
		switch(statusList.at(currentRow))
		{
		case 0: copyText+=textStatusList.at(0)+"\t"; break;
		case 1: copyText+=textStatusList.at(1)+"\t"; break;
		case 2: copyText+=textStatusList.at(2)+"\t"; break;
		case 3: copyText+=textStatusList.at(3)+"\t"; break;
		default: copyText+=textStatusList.at(4)+"\t"; break; 
		}
		copyText+=amountStrList.at(currentRow)+"\t";
		copyText+=priceStrList.at(currentRow)+"\t";
		copyText+=totalStrList.at(currentRow);

		return copyText;
	}

	if(role!=Qt::DisplayRole&&role!=Qt::ToolTipRole&&role!=Qt::ForegroundRole&&role!=Qt::TextAlignmentRole&&role!=Qt::BackgroundRole)return QVariant();

	if(role==Qt::TextAlignmentRole)return 0x0084;

	if(role==Qt::ForegroundRole)
	{
		switch(indexColumn)
		{
		case 1: return typesList.at(currentRow)?baseValues.appTheme.red:baseValues.appTheme.blue;
		default: break;
		}
		return baseValues.appTheme.black;
	}

	if(role==Qt::BackgroundRole)
	{
		switch(statusList.at(currentRow))
		{
			case 0: return baseValues.appTheme.lightRed;
			case 2: //return baseValues.appTheme.lightGreen;
			case 3: return baseValues.appTheme.lightRedGreen;
			case 4: return baseValues.appTheme.red;
			default: break;
		}
		return QVariant();
	}

	switch(indexColumn)
	{
	case -1://Counter
		{
			if(role==Qt::ToolTipRole)return oidList.at(currentRow);
			return oidList.count()-currentRow;
		}
		break;
	case 0:
		{//Date
			return dateStrList.at(currentRow);
		}
		break;
	case 1:
		{//Type
			return typesList.at(currentRow)?textAsk:textBid;
		}
		break;
	case 2:
		{//Status
			switch(statusList.at(currentRow))
			{
			case 0: return textStatusList.at(0); break;
			case 1: return textStatusList.at(1); break;
			case 2: return textStatusList.at(2); break;
			case 3: return textStatusList.at(3); break;
			default: return textStatusList.at(4); break; 
			}
		}
		break;
	case 3:
		{//Amount
			return amountStrList.at(currentRow);
		}
		break;
	case 4:
		{//Price
			return priceStrList.at(currentRow);
		}
		break;
	case 5:
		{//Total
			return totalStrList.at(currentRow);
		}
		break;
	case 6:
		{//X
			return QVariant();
		}
	default: break;
	}
	return QVariant();
}

void OrdersModel::ordersCancelAll(QByteArray pair)
{
	for(int n=oidList.count()-1;n>=0;n--)
		if(statusList.at(n)&&(pair.isEmpty()||symbolList.at(n)==pair))
			emit cancelOrder(oidList.at(n));
}

void OrdersModel::ordersCancelBids(QByteArray pair)
{
	for(int n=oidList.count()-1;n>=0;n--)
		if(statusList.at(n)&&typesList.at(n)==false&&(pair.isEmpty()||symbolList.at(n)==pair))
			emit cancelOrder(oidList.at(n));
}

void OrdersModel::ordersCancelAsks(QByteArray pair)
{
	for(int n=oidList.count()-1;n>=0;n--)
		if(statusList.at(n)&&typesList.at(n)==true&&(pair.isEmpty()||symbolList.at(n)==pair))
			emit cancelOrder(oidList.at(n));
}

void OrdersModel::setOrderCanceled(QByteArray oid)
{
	for(int n=0;n<oidList.count();n++)
		if(oidList.at(n)==oid)
		{
			statusList[n]=0;

			if(symbolList.at(n)==baseValues.currentPair.currSymbol)
			{
			if(typesList.at(n))currentAsksPrices.remove(priceList.at(n));
			else currentBidsPrices.remove(priceList.at(n));
			}
			break;
		}

	emit dataChanged(index(0,0),index(oidList.count()-1,columnsCount-1));
}

QVariant OrdersModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(orientation!=Qt::Horizontal)return QVariant();
	if(role==Qt::TextAlignmentRole)return 0x0084;

	if(role==Qt::SizeHintRole)
	{
		switch(section)
		{
		case 0: return QSize(countWidth,defaultHeightForRow);//Counter
		case 1: return QSize(dateWidth,defaultHeightForRow);//Date
		case 2: return QSize(typeWidth,defaultHeightForRow);//Type
		case 3: return QSize(statusWidth,defaultHeightForRow);//Status
		case 7: return QSize(defaultHeightForRow,defaultHeightForRow);//X
		}
		return QVariant();
	}

	if(role!=Qt::DisplayRole)return QVariant();
	if(headerLabels.count()!=columnsCount)return QVariant();

	return headerLabels.at(section);
}

Qt::ItemFlags OrdersModel::flags(const QModelIndex &) const
{
	return Qt::ItemIsSelectable|Qt::ItemIsEnabled;
}

void OrdersModel::setHorizontalHeaderLabels(QStringList list)
{
	list<<"-";
	if(list.count()!=columnsCount)return;

	textAsk=julyTr("ORDER_TYPE_ASK","ask");
	textBid=julyTr("ORDER_TYPE_BID","bid");

	textStatusList[0]=julyTr("ORDER_STATE_CANCELED",textStatusList.at(0));
	textStatusList[1]=julyTr("ORDER_STATE_OPEN",textStatusList.at(1));
	textStatusList[2]=julyTr("ORDER_STATE_PENDING",textStatusList.at(2));
	textStatusList[3]=julyTr("ORDER_STATE_POST-PENDING",textStatusList.at(3));
	textStatusList[4]=julyTr("ORDER_STATE_INVALID",textStatusList.at(4));

	dateWidth=qMax(qMax(textFontWidth(QDateTime(QDate(2000,12,30),QTime(23,59,59,999)).toString(baseValues.dateTimeFormat)),textFontWidth(QDateTime(QDate(2000,12,30),QTime(12,59,59,999)).toString(baseValues.dateTimeFormat))),textFontWidth(list.at(0)))+10;
	typeWidth=qMax(qMax(textFontWidth(textAsk),textFontWidth(textBid)),textFontWidth(list.at(1)))+10;

	for(int n=0;n<4;n++)statusWidth=qMax(textFontWidth(textStatusList.at(n)),textFontWidth(textStatusList.at(n+1)));
	statusWidth+=10;

	headerLabels=list;
	emit headerDataChanged(Qt::Horizontal, 0, columnsCount-1);
}

QModelIndex OrdersModel::index(int row, int column, const QModelIndex &parent) const
{
	if(!hasIndex(row, column, parent))return QModelIndex();
	return createIndex(row,column);
}

QModelIndex OrdersModel::parent(const QModelIndex &) const
{
	return QModelIndex();
}

int OrdersModel::getRowNum(int row){if(row<0||row>=oidList.count())return 0; return oidList.count()-row-1;}
quint32 OrdersModel::getRowDate(int row){if(row<0||row>=dateList.count())return 0; return dateList.at(getRowNum(row));}
QByteArray OrdersModel::getRowOid(int row){if(row<0||row>=oidList.count())return 0; return oidList.at(getRowNum(row));}
int OrdersModel::getRowType(int row){if(row<0||row>=typesList.count())return 0; return typesList.at(getRowNum(row))?1:0;}
int OrdersModel::getRowStatus(int row){if(row<0||row>=statusList.count())return 0; return statusList.at(getRowNum(row));}
double OrdersModel::getRowPrice(int row){if(row<0||row>=priceList.count())return 0.0;return priceList.at(getRowNum(row));}
double OrdersModel::getRowVolume(int row){if(row<0||row>=amountList.count())return 0.0;return amountList.at(getRowNum(row));}
double OrdersModel::getRowTotal(int row){if(row<0||row>=statusList.count())return 0; return statusList.at(getRowNum(row));}
