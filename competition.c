#pragma config(Sensor, 	in1, 		IR_FL, 	sensorAnalog)
#pragma config(Sensor, 	in2, 		IR_FR, 	sensorAnalog)
#pragma config(Sensor, 	in3, 		IR_B, 	sensorAnalog)
#pragma config(Sensor, 	in5, 		LSH_FL, sensorAnalog)
#pragma config(Sensor,  in6, 		LSH_FR, sensorAnalog)
#pragma config(Sensor,	in7, 		LSH_F, 	sensorAnalog)
#pragma config(Sensor, 	in8, 		SSH_B, 	sensorAnalog)
#pragma config(Sensor, 	dgtl1, 	E_L,   	sensorQuadEncoder)
#pragma config(Sensor, 	dgtl3, 	E_R,   	sensorQuadEncoder)
#pragma config(Sensor, 	dgtl5, 	LS_L, 	sensorTouch)
#pragma config(Sensor, 	dgtl6, 	LS_R, 	sensorTouch)
#pragma config(Sensor, 	dgtl7, 	LS_B, 	sensorTouch)
#pragma config(Sensor, 	dgtl8, 	LS_C, 	sensorTouch)
#pragma config(Sensor, 	dgtl9, 	DCP_1, 	sensorDigitalIn)
#pragma config(Sensor, 	dgtl10, DCP_2, 	sensorDigitalIn)
#pragma config(Sensor, 	dgtl11, DCP_3, 	sensorDigitalIn)
#pragma config(Sensor, 	dgtl12, DCP_4, 	sensorDigitalIn)
#pragma config(Motor, 	port2, 	M_L, 		tmotorServoStandard, openLoop, reversed)
#pragma config(Motor, 	port3, 	M_R, 		tmotorServoStandard, openLoop)
#pragma config(Motor, 	port4, 	M_C, 		tmotorServoStandard, openLoop, reversed)
//*!!Code automatically generated by 'ROBOTC' configuration wizard !!*//

// 'L' or 'R', IMPORTANT TO ADJUST THIS FOR EVERY ROUND!!!
// const char
char starting_side = 'R';

#define N 0
#define NE 45
#define E 90
#define SE 135
#define S 180
#define SW 225
#define W 270
#define NW 315

typedef enum
{
	STATE_INIT, STATE_SEARCH_BALL, STATE_MOVE_TO_BALL,
	STATE_DELIVER_BALL, STATE_DISPOSE_BALL, STATE_ERROR,
	STATE_SHUTDOWN
}
State;

typedef struct
{
	float x;
	float y;
}
Point;

void Point_set(Point *p, float x, float y)
{
	p->x = x; p->y = y;
}

void Point_get(Point *p, float *x, float *y)
{
	*x = p->x; *y = p->y;
}

void Point_increment(Point *p, float x, float y)
{
	p->x += x; p->y += y;
}

void Point_print(Point *p)
{
	writeDebugStreamLine("(%f, %f)", p->x, p->y);
}

const float PI = 3.14159265358979323846;
const float dist_bet_wheels = 19.214;  // cm
const float wheel_diameter = 6.985; // cm, 2.75 in
const float wheel_radius = wheel_diameter / 2.0; // cm
const float wheel_circumference = wheel_diameter * PI; // cm
const float wheel_motor_rpm = 100.0; // rpm

// --------------------------------------
// ----- Begin: Strategy Parameters -----
// --------------------------------------

float point_data [6][2] = {
	{35, 40},
	{35, 80},
	{67.5, 40},
	{67.5, 80},
	{100, 40},
	{100, 80}
};

const int left_path[2] = {2, 3};
const int right_path[2] = {3, 2};
const int* strategy_path = NULL;

Point delivery_location = {0, 0};

int ball_counter = 0;
int global_stop = 0;
bool resume_search = true;

bool has_cross = false;
bool detect_flag = true;
bool flag_move_out_front = false;
bool flag_move_out_back = false;
bool flag_safe_move_back = false;

float rel_ball_distance = 0;

// james var
int current_spinsesarch_rotation = 0;
int conveyor_runtime = 0

State current_state = STATE_INIT;
int current_search_index = 0;

Point current_robot_location = {0, 0};
float current_robot_orientation = 0;

// ------------------------------------
// ----- End: Strategy Parameters -----
// ------------------------------------

// --------------------------------------------------------
// ----- Begin: Math  and Additional Helper Functions -----
// --------------------------------------------------------

float degToRad(float angle)
{
	// Convert angle in degree to radian
	return angle * PI / 180.0;
}

float radToDeg(float angle)
{
	// Convert angle in radian to degree
	return angle * 180.0 / PI;
}

float scale(float value, float min, float max)
{
	// Scale value according to max-min normalization
	return (value - min) / (max - min);
}

float calcWheelRPM(float speed_pct)
{
	// Calculate wheel RPM speed from percentage value of the maximum value
	return wheel_motor_rpm * speed_pct / 100.0;
}

float calcWheelLinearVel(float wheel_rpm)
{
	// Calculate wheel linear velocity from wheel RPM speed
	return wheel_diameter * wheel_rpm * PI / 60.0;
}

float calcPointDistance(Point p1, Point p2)
{
	// Calculate distance to p2 as viewed from p1
	return sqrt(pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2));
}

float calcPointAngle(Point p1, Point p2)
{
	// Calculate direction (angle) to p2 as viewed from p1
	float angle = radToDeg(atan2(p2.y - p1.y, p2.x - p1.x));
	return angle >= 0.0 ? angle : angle + 360.0;
}

void updateLocation(float distance)
{
	// Update location variable according to distance moved
	float angle = degToRad(current_robot_orientation);
	current_robot_location.x += distance * cos(angle);
	current_robot_location.y += distance * sin(angle);
}

void updateOrientation(float angle)
{
	// Update orientation variable according to distance moved
	current_robot_orientation = (current_robot_orientation + angle) % 360;
}

void whereAmI()
{
	// Print current location and orientation
	writeDebugStream("Location: ");
	Point_print(&current_robot_location);
	writeDebugStreamLine("Orientation: %d", current_robot_orientation);
}

// -----------------------------------------------------
// ----- End: Math and Additional Helper Functions -----
// -----------------------------------------------------


// -----------------------------------
// ----- Begin: Sensor Functions -----
// -----------------------------------

float readSharpSensor(tSensors sensor)
{
	// Read sharp distance sensor output value in cm
	int readings = SensorValue[sensor];

	if (sensor == LSH_FR)
	{
		if (readings > 1924.969726)             return 0.0;
		else if (readings > 1626.199707)        return 12.5 - scale(readings, 1626.199707, 1924.969726) * 2.5;
		else if (readings > 1403.709960)        return 15.0 - scale(readings, 1403.709960, 1626.199707) * 2.5;
		else if (readings > 1224.489746)        return 17.5 - scale(readings, 1224.489746, 1403.709960) * 2.5;
		else if (readings > 1081.429687)        return 20.0 - scale(readings, 1081.429687, 1224.489746) * 2.5;
		else if (readings > 987.4399410)        return 22.5 - scale(readings, 987.4399410, 1081.429687) * 2.5;
		else if (readings > 891.3098140)        return 25.0 - scale(readings, 891.3098140, 987.4399410) * 2.5;
		else if (readings > 827.1198730)        return 27.5 - scale(readings, 827.1198730, 891.3098140) * 2.5;
		else if (readings > 770.2800290)        return 30.0 - scale(readings, 770.2800290, 827.1198730) * 2.5;
		else if (readings > 716.1499020)        return 32.5 - scale(readings, 716.1499020, 770.2800290) * 2.5;
		else if (readings > 667.1499020)        return 35.0 - scale(readings, 667.1499020, 716.1499020) * 2.5;
		else if (readings > 626.5598140)        return 37.5 - scale(readings, 626.5598140, 667.1499020) * 2.5;
		else if (readings > 587.7199700)        return 40.0 - scale(readings, 587.7199700, 626.5598140) * 2.5;
		else if (readings > 550.6799310)        return 42.5 - scale(readings, 550.6799310, 587.7199700) * 2.5;
		else if (readings > 501.8599850)        return 45.0 - scale(readings, 501.8599850, 550.6799310) * 2.5;
		else if (readings > 473.0999750)        return 47.5 - scale(readings, 473.0999750, 501.8599850) * 2.5;
		else                                    return 100.0;
	}

	else if (sensor == LSH_FL)
	{
		if (readings > 1856.219726)             return 0.0;
		else if (readings > 1563.020019)        return 12.5 - scale(readings, 1563.020019, 1856.219726) * 2.5;
		else if (readings > 1353.819824)        return 15.0 - scale(readings, 1353.819824, 1563.020019) * 2.5;
		else if (readings > 1179.099609)        return 17.5 - scale(readings, 1179.099609, 1353.819824) * 2.5;
		else if (readings > 1040.739746)        return 20.0 - scale(readings, 1040.739746, 1179.099609) * 2.5;
		else if (readings > 942.1899410)        return 22.5 - scale(readings, 942.1899410, 1040.739746) * 2.5;
		else if (readings > 851.6499020)        return 25.0 - scale(readings, 851.6499020, 942.1899410) * 2.5;
		else if (readings > 782.4199210)        return 27.5 - scale(readings, 782.4199210, 851.6499020) * 2.5;
		else if (readings > 721.1398920)        return 30.0 - scale(readings, 721.1398920, 782.4199210) * 2.5;
		else if (readings > 675.4799800)        return 32.5 - scale(readings, 675.4799800, 721.1398920) * 2.5;
		else if (readings > 624.7099600)        return 35.0 - scale(readings, 624.7099600, 675.4799800) * 2.5;
		else if (readings > 583.0898430)        return 37.5 - scale(readings, 583.0898430, 624.7099600) * 2.5;
		else if (readings > 545.3598630)        return 40.0 - scale(readings, 545.3598630, 583.0898430) * 2.5;
		else if (readings > 516.4099120)        return 42.5 - scale(readings, 516.4099120, 545.3598630) * 2.5;
		else if (readings > 472.7698970)        return 45.0 - scale(readings, 472.7698970, 516.4099120) * 2.5;
		else if (readings > 436.3699950)        return 47.5 - scale(readings, 436.3699950, 472.7698970) * 2.5;
		else                                    return 100.0;
	}

	else if (sensor == LSH_F)
	{
		if (readings > 1377.529785)             return 0.0;
		else if (readings > 1222.739746)        return 12.5 - scale(readings, 1222.739746, 1377.529785) * 2.5;
		else if (readings > 1109.540039)        return 15.0 - scale(readings, 1109.540039, 1222.739746) * 2.5;
		else if (readings > 1005.609863)        return 17.5 - scale(readings, 1005.609863, 1109.540039) * 2.5;
		else if (readings > 922.5798330)        return 20.0 - scale(readings, 922.5798330, 1005.609863) * 2.5;
		else if (readings > 871.3298330)        return 22.5 - scale(readings, 871.3298330, 922.5798330) * 2.5;
		else if (readings > 794.2600090)        return 25.0 - scale(readings, 794.2600090, 871.3298330) * 2.5;
		else if (readings > 789.6599120)        return 27.5 - scale(readings, 789.6599120, 794.2600090) * 2.5;
		else if (readings > 735.8698730)        return 30.0 - scale(readings, 735.8698730, 789.6599120) * 2.5;
		else if (readings > 688.7600090)        return 32.5 - scale(readings, 688.7600090, 735.8698730) * 2.5;
		else if (readings > 692.0200190)        return 35.0 - scale(readings, 692.0200190, 688.7600090) * 2.5;
		else if (readings > 622.1799310)        return 37.5 - scale(readings, 622.1799310, 692.0200190) * 2.5;
		else if (readings > 578.0998530)        return 40.0 - scale(readings, 578.0998530, 622.1799310) * 2.5;
		else if (readings > 623.0998530)        return 42.5 - scale(readings, 623.0998530, 578.0998530) * 2.5;
		else if (readings > 510.1300040)        return 45.0 - scale(readings, 510.1300040, 623.0998530) * 2.5;
		else if (readings > 496.9499510)        return 47.5 - scale(readings, 496.9499510, 510.1300040) * 2.5;
		else                                    return 100.0;
	}

	else if (sensor == SSH_B)
	{
		if (readings > 2856.519531)             return 0.0;
		else if (readings > 2846.559570)        return 12.5 - scale(readings, 2846.559570, 2856.519531) * 2.5;
		else if (readings > 2837.109375)        return 15.0 - scale(readings, 2837.109375, 2846.559570) * 2.5;
		else if (readings > 2797.149414)        return 17.5 - scale(readings, 2797.149414, 2837.109375) * 2.5;
		else if (readings > 1088.520019)        return 20.0 - scale(readings, 1088.520019, 2797.149414) * 2.5;
		else if (readings > 1036.579589)        return 22.5 - scale(readings, 1036.579589, 1088.520019) * 2.5;
		else if (readings > 953.5000000)        return 25.0 - scale(readings, 953.5000000, 1036.579589) * 2.5;
		else if (readings > 885.8898920)        return 27.5 - scale(readings, 885.8898920, 953.5000000) * 2.5;
		else if (readings > 835.6599120)        return 30.0 - scale(readings, 835.6599120, 885.8898920) * 2.5;
		else if (readings > 735.4399410)        return 35.0 - scale(readings, 735.4399410, 835.6599120) * 5.0;
		else if (readings > 659.1699210)        return 40.0 - scale(readings, 659.1699210, 735.4399410) * 5.0;
		else if (readings > 607.4099120)        return 45.0 - scale(readings, 607.4099120, 659.1699210) * 5.0;
		else if (readings > 543.3698730)        return 50.0 - scale(readings, 543.3698730, 607.4099120) * 5.0;
		else                                    return 100.0;
	}
	return(100.0)
}

bool readLimitSwitch(tSensors sensor)
{
	// Read limit switch output value - True / False
	return SensorValue[sensor] == 1;
}

bool readIRSensor(tSensors sensor)
{
	// Read IR sensor output value in cm
	return SensorValue[sensor] < 1000;
}

void readCompass(int* output)
{
	// Read compass direction output value in 8 directions: N, NE, E, SE, S, SW, W, NW
	string input = "";

	tSensors ports[] = {DCP_1, DCP_2, DCP_3, DCP_4};

	for (int i = 0; i < 4; i++)	strcat(input, (SensorValue[ports[i]] == 1) ? "1" : "0");

	if (input == "0111")			*output = N;
	else if (input == "0110") 		*output = NE;
	else if (input == "1110") 		*output = E;
	else if (input == "1100")		*output = SE;
	else if (input == "1101") 		*output = S;
	else if (input == "1001")		*output = SW;
	else if (input == "1011")		*output = W;
	else if (input == "0011")		*output = NW;
}

bool checkObject()
{
	// Check if the object is a ball, not another vehicle or boundary - True / False
	double ball_distance = 0.0;
	double FL_distance = 0.0;
	double FR_distance = 0.0;
	int i;
	double num_step = 10;

	for (i = 0; i < num_step; i++)
	{
		ball_distance += readSharpSensor(LSH_F);
		FL_distance += readSharpSensor(LSH_FL);
		FR_distance += readSharpSensor(LSH_FR);
		sleep(10);
	}

	ball_distance /=  num_step;
	FL_distance /= num_step;
	FR_distance /= num_step;

	writeDebugStreamLine("Check obj: %lf %lf %lf",FL_distance,ball_distance,FR_distance);

	// int other_distance = (FL_distance < FR_distance ? FL_distance : FR_distance);
	int other_distance = (int)(FL_distance+FR_distance)/2;

	if (ball_distance < 30 && other_distance > 40){
		rel_ball_distance = ball_distance;
		return true;
	}
	else{
		return false;
	}
}


// ---------------------------------
// ----- End: Sensor Functions -----
// ---------------------------------


// ----------------------------------------
// ----- Begin: Basic Motor Functions -----
// ----------------------------------------

void runMotors(int speed_pct_left, int speed_pct_right)
{
	// Run wheel motors at a given speed for each motor
	motor[M_L] = speed_pct_left;
	motor[M_R] = speed_pct_right;
}

void stopMotors()
{
	// Stop wheel motors immediately
	motor[M_L] = 0.0;
	motor[M_R] = 0.0;
}

void runConveyor(int speed_pct)
{
	// Run the converyor belt at a given speed
	motor[M_C] = speed_pct;
}

void stopConveyor()
{
	// Stop the converyor belt immediately
	motor[M_C] = 0.0;
}

// --------------------------------------
// ----- End: Basic Motor Functions -----
// --------------------------------------

// -----------------------------------------
// ----- Begin: Basic Motion Functions -----
// -----------------------------------------

void move(int duration, int speed_pct_left, int speed_pct_right, int speed_pct_conveyor=0)
{
	// Move left and right motors or conveyor simultaneously for a given duration at a given speed
	clearTimer(T1);
	while (time1[T1] < duration)
	{
		if (speed_pct_conveyor)	runConveyor(speed_pct_conveyor);
		else   									runMotors(speed_pct_left, speed_pct_right);
		sleep(100);
	}
	if (speed_pct_conveyor) 		stopConveyor();
	else							stopMotors();
}

void linearMoveRandom(string direction, float distance, int speed_pct) {
		// Move linearly for a given direction and distance at a given speed
    int E_L_init_value = SensorValue[E_L];
    int E_R_init_value = SensorValue[E_R];
    int direction_int = (direction == "FWD") ? 1 : (direction == "BWD") ? -1 : 0;

	float offset = 10; // before change: 20
	int slow_speed_pct = 30; // before change: 20
	float final_offset = 0.5; // before change: 2.2
	float speed_inc = 0;
	stopConveyor();
	runConveyor(100);
	while (degToRad(fabs(SensorValue[E_L] - E_L_init_value)) * wheel_radius < distance - offset)
	{
		if (readLimitSwitch(LS_C)) stopConveyor();
		if (direction_int == -1)			runMotors(((int) speed_inc*0.8) * direction_int, ((int) speed_inc) * direction_int);
		else								runMotors(((int) speed_inc) * direction_int, (int) speed_inc * 0.95 * direction_int);
		if (speed_inc < speed_pct)	speed_inc += 0.01;
	}
	stopMotors();
	while (degToRad(fabs(SensorValue[E_L] - E_L_init_value)) * wheel_radius < distance - final_offset)
	{
		if (readLimitSwitch(LS_C)) stopConveyor();
		if (direction_int == -1)			runMotors((slow_speed_pct*0.8) * direction_int, (slow_speed_pct) * direction_int);
		else								runMotors((slow_speed_pct) * direction_int, slow_speed_pct * 0.95 * direction_int);
	}

    stopMotors();

    // update location
    if (direction == "FWD")						updateLocation(distance);
    else if (direction == "BWD")				updateLocation(-distance);
}

void linearMove(string direction, float distance, int speed_pct, bool encoder=true) {
	// Move linearly for a given direction and distance at a given speed
    int E_L_init_value = SensorValue[E_L];
    int E_R_init_value = SensorValue[E_R];
    int direction_int = (direction == "FWD") ? 1 : (direction == "BWD") ? -1 : 0;
    if (encoder)
    {
        // Encoder-based control
        float offset = 10; // before change: 20
        int slow_speed_pct = 30; // before change: 20
        float final_offset = 0.5; // before change: 2.2
        float speed_inc = 0;

        while (degToRad(fabs(SensorValue[E_L] - E_L_init_value)) * wheel_radius < distance - offset)
        {
            if (direction_int == -1)		runMotors(((int) speed_inc*0.8) * direction_int, ((int) speed_inc) * direction_int);
            else												runMotors(((int) speed_inc) * direction_int, (int) speed_inc * 0.95 * direction_int);
            if (speed_inc < speed_pct)	speed_inc += 0.01;
        }
        stopMotors();
        while (degToRad(fabs(SensorValue[E_L] - E_L_init_value)) * wheel_radius < distance - final_offset)
        {
            if (direction_int == -1)		runMotors((slow_speed_pct*0.8) * direction_int, (slow_speed_pct) * direction_int);
            else												runMotors((slow_speed_pct) * direction_int, slow_speed_pct * 0.95 * direction_int);
        }
    }
    else
    {
        // Time-based control
        float duration = distance / calcWheelLinearVel(calcWheelRPM(speed_pct));
        move(duration, speed_pct * direction_int, speed_pct * direction_int, 0);
    }
    stopMotors();

    // update location
    if (direction == "FWD")							updateLocation(distance);
    else if (direction == "BWD")				updateLocation(-distance);
}

void rotationalMove(string direction, float angle, int speed_pct, bool encoder=true)
{
	// Rotate for a given direction and angle at a given speed
	int E_L_init_value = SensorValue[E_L];
	int E_R_init_value = SensorValue[E_R];
	int direction_int = (direction == "CW") ? 1 : (direction == "CCW") ? -1 : 0;
	float wheel_angle = dist_bet_wheels * angle / wheel_diameter;
	if (angle == 180) {direction = 1;}
	if (encoder)
	{
		// Encoder-based control
		float offset = 0;
		if (direction_int == 1)
		{
			if (angle == 10)			offset = 0.763; // offset = 0.9;
			else if (angle == 90)		offset = 1.095; 	// offset = 1.12;
			else if (angle == 180)		offset = 1.2; // offset = 1.13;
			else										return;
		}
		else if (direction_int == -1)
		{
			if (angle == 10)        	offset = 0.945; // offset = 0.828; newer: 1.045
			else if (angle == 90) 		offset = 1.285; // offset = 1.215
			else if (angle == 180)		offset = 1.148; // offset = 1.25
			else										return;
		}
		else	return;

		while (fabs(SensorValue[E_L] - E_L_init_value) < wheel_angle * offset)
		{
			runMotors(speed_pct * direction_int, -speed_pct * direction_int);
		}

	}
	else
	{
		// Time-based control
		float duration = degToRad(angle) * dist_bet_wheels / calcWheelLinearVel(calcWheelRPM(speed_pct)) / 2;
		move(duration, speed_pct * direction_int, -speed_pct * direction_int, 0);
	}
	stopMotors();

	// update orientation
	if (direction == "CW")				updateOrientation(-angle);
	else if (direction == "CCW")		updateOrientation(angle);
}

// ---------------------------------------
// ----- End: Basic Motion Functions -----
// ---------------------------------------

// --------------------------------------------
// ----- Begin: Advanced Motion Functions -----
// --------------------------------------------

bool spinSearch() {
	// Perform spin search around the location
	float current_angle = 0;
	while (current_spinsesarch_rotation < 36)
	{
		sleep(100);
		if (checkObject()) 	return true;
		string direction = "CCW";
		rotationalMove(direction, 10, 30);
		current_angle += 10;
		current_spinsesarch_rotation += 1;
	}
	current_spinsesarch_rotation = 0;
	return false;
}

void align(float angle) {
	// Rotate the robot so that the forward direction aligns with the given angle
	string direction;
	float diff = angle - current_robot_orientation;

	if (diff < 0)		diff += 360;

	if (diff <= 180)
	{
		direction = "CCW";
		rotationalMove(direction, diff, 30);
	}
	else
	{
		direction = "CW";
		rotationalMove(direction, 360 - diff, 30);
	}
}

void moveTo(Point A) {
	// Move the robot to Point A from the current location
	float x = A.x;
	float y = A.y;
	float x0 = current_robot_location.x;
	float y0 = current_robot_location.y;
	float dx = x - x0;
	float dy = y - y0;
	float ax = (dx >= 0) ? 0 : 180;
	float ay = (dy >= 0) ? 90 : 270;
	float a = current_robot_orientation;
	float dax = fabs(a - ax);
	float day = fabs(a - ay);
	string direction;

	if (fabs(dy) <= 1)
	{
		align(ax);
		direction = "FWD";
		linearMove(direction, fabs(dx), 50);
	}
	else if (fabs(dx) <= 1)
	{
		align(ay);
		direction = "FWD";
		linearMove(direction, fabs(dy), 50);
	}
	else
	{
		if (dax >= day)
		{
			align(ay);
			direction = "FWD";
			linearMove(direction, fabs(dy), 50);
			sleep(100);
			align(ax);
			direction = "FWD";
			linearMove(direction, fabs(dx), 50);
			sleep(100);
		}
		else
		{
			align(ax);
			direction = "FWD";
			linearMove(direction, fabs(dx), 50);
			sleep(100);
			align(ay);
			direction = "FWD";
			linearMove(direction, fabs(dy), 50);
			sleep(100);
		}
	}
}

void alignCompass(int target_direction)
{
	int current_direction;
	string direction;
	readCompass(&current_direction);

	writeDebugStreamLine("%d %d", current_direction, target_direction);

	int diff = target_direction - current_direction;

	if (diff < 0) diff += 360;

	if (diff <= 180)
	{
		while (true)
		{
			direction = "CW";
			rotationalMove(direction, 10, 30);
			readCompass(&current_direction);
			if (current_direction == target_direction) return;
		}
	}
	else
	{
		while (true)
		{
			direction = "CCW";
			rotationalMove(direction, 10, 30);
			readCompass(&current_direction);
			if (current_direction == target_direction) return;
		}
	}
}

// ------------------------------------------
// ----- End: Advanced Motion Functions -----
// ------------------------------------------

// ----------------------------------------------
// --------- Begin: Additional Sub-tasks --------
// ----------------------------------------------
task rollbackConveyor() {
    clearTimer(T2);
    stopConveyor();
    conveyor_runtime = 0;
}

task moveSlowlyForward() {
	float dist;

	if(rel_ball_distance < 1){
		dist = 25;
	}
	else if (rel_ball_distance < 10){
		dist = 30;
	}
	else{
		dist = 40;
	}
    string direction = "FWD";
    writeDebugStreamLine("===== Start moving forward towards ball =====");
    linearMove(direction, dist, 30, true);
    sleep(2000);
    global_stop = 0;
    writeDebugStreamLine("===== Start moving backward away from ball =====");
    direction = "BWD";
    linearMove(direction, dist, 100, true);
    sleep(1000);
    global_stop = -1;
}

task lineCrossingCheck()
{
	bool has_ball = false;

	string direction;

	while (true)
	{
		has_ball = readLimitSwitch(LS_C);

		if (readIRSensor(IR_FL) || readIRSensor(IR_FR))
		{
			flag_move_out_front = true;
			if (!has_cross)
			{
				has_cross = true;
				stopTask(generalStrategy);
				stopTask(moveSlowlyForward);
				startTask(randomStrategy);
			}
		}
		else if (readIRSensor(IR_B) && readLimitSwitch(LS_C))
		{
			flag_move_out_back = true;
			if (!has_cross)
			{
				has_cross = true;
				stopTask(generalStrategy);
				stopTask(moveSlowlyForward);
				startTask(randomStrategy);
			}
		}
	}
}
// ----------------------------------------------
// --------- End: Additional Sub-tasks ----------
// ----------------------------------------------

// ---------------------------------------
// ----- Begin: Main State Functions -----
// ---------------------------------------

bool initFunc() {
	/*
	(1) Wait for the user to press a back switch to determine starting side
	(2) Assign starting side, delivery location, and strategy path
	(3) Set initial robot location
	*/
	while (true) {
		if (starting_side == 'L')
		{
			strategy_path = &left_path;
			delivery_location.x = 210;
			delivery_location.y = 40;
			current_robot_location.x = 220;
			current_robot_location.y = 40;
			current_robot_orientation = 180;
			return true;
		}
		else if (starting_side == 'R')
		{
			strategy_path = &right_path;
			delivery_location.x = 210;
			delivery_location.y = 80;
			current_robot_location.x = 220;
			current_robot_location.y = 80;
			current_robot_orientation = 180;
			return true;
		}
		sleep(100);
	}
}

bool searchBallFunc() {
	/*
	// (1) If the search is just started/resumed, move to 50% of the length
	// (2) Linear move to the current search circle
	// (3) Update the current search index
	// (4) Do spin search and return True if the ball is found
	// (5) Else continue the search with the updated current search index
	*/

	float x, y;
	if (resume_search)
	{
        if (conveyor_runtime > 0){
            startTask(rollbackConveyor);
        }
		string direction = "FWD";
		linearMove(direction, 100, 100);
		stopMotors();
		resume_search = false;
		whereAmI();
		writeDebugStreamLine("===== Forward 1 m =====");
	    while (conveyor_runtime > 0) {
        	sleep(10);
        }
	}
	while (true)
	{
		if (current_spinsesarch_rotation > 0) {
			current_search_index = (current_search_index - 1) % 2;
		}
		x = point_data[strategy_path[current_search_index]][0];
		y = point_data[strategy_path[current_search_index]][1];
		writeDebugStreamLine("===== Searching at Circle No. %d: x = %lf, y = %lf", strategy_path[current_search_index], x, y);

		Point p_next;
		Point_set(&p_next, x, y);

		moveTo(p_next);
		stopMotors();
		whereAmI();
		sleep(100);
		current_search_index = (current_search_index + 1) % 2;
		writeDebugStreamLine("===== Start SpinSearch at Circle No. %d: x = %lf, y = %lf", strategy_path[current_search_index], x, y);
		if (spinSearch()) return true;
	}
}

bool moveToBallFunc() {
	/*
	(1) Estimate the distance to the ball
	(2) Run the conveyor
	(3) Move forward until the ball is captured
	(4) If not, go back to STATE_SEARCH_BALL without resume search status
	*/
	global_stop = 1;
	startTask(moveSlowlyForward);
	bool flag = false;

 	clearTimer(T2);
    while (global_stop) {
        if (readLimitSwitch(LS_C)) {
        		if (!flag) {
	            flag = true;
	            conveyor_runtime = conveyor_runtime + time1[T2];
          }
          stopConveyor();
        }
        else {
        	runConveyor(100);
        }
    }

	if (!flag) {
        conveyor_runtime = conveyor_runtime + time1[T2];
        startTask(rollbackConveyor);
    }
    while (global_stop==0){
        sleep(10);
    }
	return flag;
}

bool deliverBallFunc()
{
    /*
    (1) Calculate the distance and angle to the delivery area
    (2) Move to the delivery area slowly, until one of the back switches are pressed
    */
    if (current_spinsesarch_rotation > 0){
        for(int i=current_spinsesarch_rotation; i<36; i++) {
            string direction = "CCW";
            rotationalMove(direction, 10, 30, true);
            sleep(100);
        }
        current_spinsesarch_rotation = 0;
    }


    while (true) {
    		runMotors(-75, -75);
    		if (readLimitSwitch(LS_B)) //&& readIRSensor(IR_B))
    		{
    				stopMotors();
    				break;
    		}
    }

    Point_set(&current_robot_location, delivery_location.x + 10, delivery_location.y);
    whereAmI();
    return true;
}

bool disposeBallFunc()
{
	/*
	(1) Ensure the robot stops moving
	(2) Resume conveyor belt to release the ball
	(3) Wait for the ball to be released
	*/
	stopMotors();
	move(5000, 0, 0, 100);
	string direction = "FWD";
	linearMove(direction, 10, 75);
	conveyor_runtime = conveyor_runtime + 5000;
	return true;
}

// -------------------------------------
// ----- End: Main State Functions -----
// -------------------------------------

void initOption()
{
	// State machine function for STATE_INIT
	writeDebugStreamLine("========== STATE INIT ==========");
	bool start_program = initFunc();
	if (start_program) current_state = STATE_SEARCH_BALL;
}

void searchBallOption()
{
	// State machine function for STATE_SEARCH_BALL
	writeDebugStreamLine("========== STATE SEARCH BALL ==========");
	bool found_ball = searchBallFunc();
	if (found_ball) current_state = STATE_MOVE_TO_BALL;
}

void moveToBallOption()
{
	// State machine function for STATE_MOVE_TO_BALL
	writeDebugStreamLine("========== STATE MOVE TO BALL ==========");
	bool capture_ball = moveToBallFunc();
	if (capture_ball) current_state = STATE_DELIVER_BALL;
	else current_state = STATE_SEARCH_BALL;
}

void deliverBallOption()
{
	// State machine function for STATE_DELIVER_BALL
	writeDebugStreamLine("========== STATE DELIVER BALL ==========");
	bool reach_delivery_spot = deliverBallFunc();
	if (reach_delivery_spot) current_state = STATE_DISPOSE_BALL;
}

void disposeBallOption()
{
	// State machine function for STATE_DISPOSE_BALL
	writeDebugStreamLine("========== STATE DISPOSE BALL ==========");
	bool dispose_success = disposeBallFunc();
	if (dispose_success) ball_counter++;
	if (ball_counter < 2)
	{
		current_state = STATE_SEARCH_BALL;
		resume_search = true;
	}
	else
	{
		current_state = STATE_SHUTDOWN;
	}
}

void errorOption()
{
	// State machine function for STATE_ERROR
	writeDebugStreamLine("========== STATE ERROR ==========");
	current_state = STATE_SHUTDOWN;
}

task generalStrategy()
{
   // State machine controller
	while(true)
	{
		if (readLimitSwitch(LS_L)){
			starting_side = 'L';
			writeDebugStreamLine("START ===== %c =====", starting_side);
			break;
		}
		else if (readLimitSwitch(LS_R)){
			starting_side = 'R';
			writeDebugStreamLine("START ===== %c =====", starting_side);
			break;
		}
	}

   while (true)
	{
       if (current_state == STATE_SHUTDOWN) break;

       switch (current_state)
       {
           case STATE_INIT:
               initOption();
               break;
           case STATE_SEARCH_BALL:
               searchBallOption();
               break;
           case STATE_MOVE_TO_BALL:
               moveToBallOption();
               break;
           case STATE_DELIVER_BALL:
               deliverBallOption();
               break;
           case STATE_DISPOSE_BALL:
               disposeBallOption();
               break;
           case STATE_ERROR:
               errorOption();
               break;
           default:
               current_state = STATE_ERROR;
               break;
     	}
       sleep(1000);
   }

}

task randomStrategy()
{
	writeDebugStreamLine("========== INITIALISING STATE RANDOM ==========");
	string direction;
	int current_direction;
	bool flag;
	while (true)
	{
		// hold on to the ball --> move back : can only move out from back
		if (readLimitSwitch(LS_C))
		{
			// ball is captured	
			if (flag_move_out_back)
			{
				// Yellow line is detected from the back IR sensor
				// Moving forward (to move away from the yellow line)
				flag_move_out_back = false;
				direction = "FWD";
				linearMove(direction, 20, 75);
			}
			else
			{
				writeDebugStreamLine("Moving backward to reach the delivery area");
				delay(500);
				alignCompass(S);
				delay(500);
				while (true)
				{
					runMotors(-75, -75);
					if (readLimitSwitch(LS_B) && readIRSensor(IR_B))
					{
						stopMotors();
						break;
					}
				}
				writeDebugStreamLine("======== DISPOSE BALL =========");
				disposeBallFunc();
				direction = "FWD";
				linearMoveRandom(direction, 80, 75);
			}
		}

		// other positions : can only move out from front
		else
		{
			if (flag_move_out_front)
			{
				
				// Yellow line is detected from the front IR sensor
				// Moving forward (to move away from the yellow line)
				flag_move_out_front = false;
				direction = "BWD";
				stopConveyor();
				linearMove(direction, 30, 75);
			}
			else
			{
				/* === Start of the crossing to the opposite side of the field === */
				// move to different side
				if (starting_side == 'L')
				{
					// move all to W
					alignCompass(S);
					string direction = "CW";
					rotationalMove(direction, 90, 30);
					// end move
					delay(500);
				}
				else if (starting_side == 'R')
				{
					// move all to E
					alignCompass(S);
					string direction = "CCW";
					rotationalMove(direction, 90, 30);
					// end move
					delay(500);
				}
				// to reach east edge or west edge
				while(!flag_move_out_front)
				{
					// runMotors(75, 75);
					direction = "FWD";
					linearMoveRandom(direction, 1, 75);
				}
				flag_move_out_front = true;
				direction = "BWD";
				stopConveyor();
				linearMove(direction, 30, 75);
				delay(100);
				alignCompass(S);
				delay(100);

				/* === End of crossing to the opposite side of the field === */

				// spinsearch every 5 seconds
				clearTimer(T3);
				flag_move_out_front = false;
				while(!flag_move_out_front)
				{
					if (readLimitSwitch(LS_C)) 	break;
					if (time1[T3] % 5000 == 0)
					{
						checkObject();
						flag = false;
						flag = spinSearch();
						writeDebugStreamLine("Flag status: %d", flag);
						if (flag) break;
					}

					direction = "FWD";
					linearMoveRandom(direction, 1, 75);
				}

				flag_move_out_front = true;

				// if ball is found, move to ball, align to South, and dispose
				if (flag)
				{
					moveToBallFunc();
					sleep(1000);
					alignCompass(S);
					delay(100);
					while (readLimitSwitch(LS_C))
					{
						runMotors(-50, -50);
						if (readLimitSwitch(LS_B) && readIRSensor(IR_B))
						{
							stopMotors();
							break;
						}
					}
					disposeBallFunc();
					direction = "FWD";
					linearMoveRandom(direction, 30, 75);
				}
			}
		}
	}
}

task main()
{
	startTask(generalStrategy);
	startTask(lineCrossingCheck);
	while(1);
}
