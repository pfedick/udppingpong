/*
 * dnsperftest_sensor.h
 *
 *  Created on: 21.07.2016
 *      Author: patrickf
 */

#ifndef INCLUDE_SENSORDAEMON_H_
#define INCLUDE_SENSORDAEMON_H_


class SensorDaemon
{
	private:
		void help();

	public:
		SensorDaemon();
		~SensorDaemon();
		int main(int argc, char **argv);

};



#endif /* INCLUDE_SENSORDAEMON_H_ */
