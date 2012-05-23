/**********************************************************************
**
** Margouilla Runtime Library
**
**
** Copyright (C) 2002-2003 CQ-Software.  All rights reserved.
**
**
** This file is distributed under the terms of the GNU Library 
** General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.
**
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.margouilla.com
**********************************************************************/
/* $Id: mgl_msgset.cpp,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */




#include <stdio.h>

#include "mgl_msgset.h"


mgl_msgset::mgl_msgset()
{
}

void mgl_msgset::init()
{
	_msgIdList.init();
}

void mgl_msgset::clear()
{
	_msgIdList.clear();
}

/*
mgl_msgset::mgl_msgset(mgl_id i_id1, ...)
{
	msgIdAppend(i_id1);
	//dump();
}


mgl_msgset::mgl_msgset(mgl_id i_id1)
{
	msgIdAppend(i_id1);
	dump();
}

mgl_msgset::mgl_msgset(mgl_id i_id1, mgl_id i_id2)
{
	msgIdAppend(i_id1);
	msgIdAppend(i_id2);
}

  */
mgl_msgset::mgl_msgset(mgl_id i_id1, mgl_id i_id2, mgl_id i_id3, mgl_id i_id4)
{
	if (i_id1!=-1) msgIdAppend(i_id1);
	if (i_id2!=-1) msgIdAppend(i_id2);
	if (i_id3!=-1) msgIdAppend(i_id3);
	if (i_id4!=-1) msgIdAppend(i_id4);
}

mgl_status mgl_msgset::msgIdAppend(mgl_id i_id)
{
	if (_msgIdList.append((void *)i_id)) {
		return mgl_ok;
	} else {
		return mgl_ko;
	}
}



mgl_status mgl_msgset::msgIdRemove(mgl_id i_id)
{
	if (_msgIdList.removeByPtr((void *)i_id)) {
		return mgl_ok;
	} else {
		return mgl_ko;
	}
}

long mgl_msgset::getCount()
{
	return (_msgIdList.getCount());
}

mgl_id mgl_msgset::get(long i_index)
{
	return ((mgl_id)_msgIdList.get(i_index));
}


mgl_bool   mgl_msgset::msgIdIsIn(mgl_id i_id)
{
	if (_msgIdList.getCount()<=0) {
		return mgl_false;
	}
	if (_msgIdList.getIndexByPtr((void *)i_id)>=0) {
		return mgl_true;
	} else {
		return mgl_false;
	}
}


void mgl_msgset::operator = (mgl_msgset i_param)
{
	long l_nb;
	long l_cpt;

	_msgIdList.clear();
	l_nb = i_param.getCount();
	for (l_cpt=0; l_cpt<l_nb; l_cpt++) {
		_msgIdList.append((void *)i_param.get(l_cpt));
	}
}


void mgl_msgset::dump()
{
	long l_nb;
	long l_cpt;

	l_nb = getCount();
	printf("[ ");
	for (l_cpt=0; l_cpt<l_nb; l_cpt++) {
		printf("<%ld> ", get(l_cpt));
	}
	printf(" ]\n");
}


/*
msgset(msgset &m) { ... } // copie de msgset

msgset &operator , (msgid i) {
  //ajoute l'id de message i dans l'ensemble
}

en principe, msgset((msgset)1,2,3,...) devrait marcher quelque soit le nombre d'id 
je te conseille que le constructeur et l'opérateur soit inline pour bénéficier d'une optimisation optimale et conforme au nombre exacte d'id
tu fais msgset((msgset)1|2|3) et ça marche !



  struct list {

  int value;

  list *rest;

  list (int v) : value(v),rest(0) {}
  list (int v,list &l) : value(v),rest(&l) {}

  friend list &cons (int v,list &l) { return *(new list (v,l)); }

  //// ce qui nous intéresse commence ici 
  list (list &l) : value(l.value),rest(l.rest) {}

  list &operator | (int v) { return cons (v,*this); }
  //// et termine ici

  void dump () { printf("%d\n",value); if (rest) rest->dump(); }

};

int main(int argc, char* argv[])
{
  list l((list)1|2|3);
  l.dump ();
	return 0;
}


*/


