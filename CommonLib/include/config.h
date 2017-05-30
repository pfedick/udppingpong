/*
 * config.h
 *
 *  Created on: 30.05.2017
 *      Author: patrickf
 */

#ifndef COMMONLIB_INCLUDE_CONFIG_H_
#define COMMONLIB_INCLUDE_CONFIG_H_
#include <ppl7.h>

class BotConfig
{
	public:
		int				Id;
		ppl7::String 	Name;
		ppl7::IPAddress Addr;
		int				Port;
		bool			feature_proxy;
		bool			feature_sensor;
		bool			feature_loadgenerator;
};



#endif /* COMMONLIB_INCLUDE_CONFIG_H_ */
