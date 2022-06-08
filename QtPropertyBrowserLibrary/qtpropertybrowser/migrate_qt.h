/*
 * =====================================================================================
 *
 *       Filename:  migrate_qt.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  12/10/2021 08:31:49 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Pished Bunnun (pished.bunnun@nectec.or.th), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <QtCore/qglobal.h>

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)

#define Q_DISABLE_MOVE(Class) \
    Class(Class &&) = delete; \
    Class &operator=(Class &&) = delete;

#define Q_DISABLE_COPY_MOVE(Class) \
    Q_DISABLE_COPY(Class) \
    Q_DISABLE_MOVE(Class)

#endif
