/**************************************************************************
    Robô com pernas humanóides e 6 graus de liberdade em cada perna
    Feito por: Oscar E. Ramos Ponce
    Estudante do PPGEE, Universidade Federal do Rio Grande do Sul, Brasil
        Este Progrâma apresenta o robô com todos os ângulos fixos em 0
        graus (posição canónica). Não existe movimento
***************************************************************************/

// Utiliza robModel.cpp e robPrimitives.cpp

#include <ode/ode.h>
#include <drawstuff/drawstuff.h>
#include "../include/robModel.h"
#include "../include/robFunctions.h"

#define SIM_STEP 0.01


dWorldID    world;                          // Variable for world (where the objects will be drawn)
dSpaceID    space;  			            // Varialbe for space (needed for the "geometry" that describes collision detection)
dsFunctions fn;             	            // "drawstuff" structure: contains information about functions for drawing

// Ground and Contact group
static dGeomID ground;				        // Variable for ground geometry
static dJointGroupID contactgroup;	        // Contact group for collision

// Joints
dJointID jointR[MAX_JOINTS];         // Joints for the Right leg (defined in robCreateDraw.cpp)
dJointID jointL[MAX_JOINTS];         // Joints for the Left leg  (defined in robCreateDraw.cpp)

// Desired (target) angles
static double target_angleR[MAX_JOINTS] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; //Angles for Right leg initialized to 0
static double target_angleL[MAX_JOINTS] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; //Angles for Left leg initialized to 0

//Joint angles (left and right feet)
double TOT_ang_R[6][TLENG], TOT_ang_L[6][TLENG];    // Variable for joint angles (6), TLENG: # of different values in time

//status of collision
bool innerCollision;

void setDrawStuff();



/***************************************************************************************************************
 ********************* COLLISION DETECTION *********************************************************************
 ***************************************************************************************************************/

static void nearCallback(void *data, dGeomID o1, dGeomID o2)        //o1,o2: geometries likely to collide
{
     // inner collision detecting
    innerCollision = innerCollision || (o1 != ground && o2 != ground);

    const int N = 30;
    dContact contact[N];

    // Don’t do anything if the two bodies are connected by a joint
    dBodyID b1 = dGeomGetBody(o1);
    dBodyID b2 = dGeomGetBody(o2);
    if (b1 && b2 && dAreConnected (b1,b2)) return;

    int isGround = ((ground == o1) || (ground == o2));              //Collision with ground
    int n =  dCollide(o1,o2,N,&contact[0].geom,sizeof(dContact));   //n: Number of collision points

    //If there is a collision with the ground
    if (isGround)
    {
        for (int i = 0; i < n; i++)
        {   contact[i].surface.mode = dContactSoftERP | dContactSoftCFM;
            contact[i].surface.mu   = dInfinity; //Coulomb friction coef
            contact[i].surface.soft_erp = 0.2;	// 1.0 ideal
            contact[i].surface.soft_cfm = 1e-4;	// 0.0 ideal

            dJointID c = dJointCreateContact(world,contactgroup,&contact[i]);
            dJointAttach(c,dGeomGetBody(contact[i].geom.g1),
                           dGeomGetBody(contact[i].geom.g2));
        }
    }
}


/**************************************************************************************************************
 ********************* CONTROL *********************************************************************************
 **************************************************************************************************************
 control(): Assigns the angle to the joint:
              target_angleR/L[i] to jointR/L[i]
*/

void control()
{
    double k1 =  10.0,  fMax  = 100.0;                                      // k1:gain, fMax: max torque [Nm]

    for(int i=0;i<MAX_JOINTS;i++)
    {
        double current_angle = dJointGetHingeAngle(jointR[i]);              // current joint
        double z = target_angleR[i]*M_PI/180 - current_angle;               // z = target – current
        // Set angular velocity for the joint: w = k1(th_tarjet-th_curr)
        dJointSetHingeParam(jointR[i],dParamVel,k1*z);
        dJointSetHingeParam(jointR[i],dParamFMax,fMax);                     // max torque
    }
    for(int i=0;i<MAX_JOINTS;i++)
    {
        double current_angle = dJointGetHingeAngle(jointL[i]);              // current joint
        double z = target_angleL[i]*M_PI/180 - current_angle;               // z = target – current
        // Set angular velocity for the joint: w = k1(th_tarjet-th_curr)
        dJointSetHingeParam(jointL[i],dParamVel,k1*z);
        dJointSetHingeParam(jointL[i],dParamFMax,fMax);                     // max torque
    }
}


/***************************************************************************************************************
 ********************* FUNCTION FOR STARTING THE SUMULATION ****************************************************
 ***************************************************************************************************************/

void start()
{
//    static float xyz[3] = {1.5, -1.5, 0.8};    // View position (x, y, z) [m]
//    static float hpr[3] = {136.0, 0.0, 0.0};    // View direction （head,pitch,roll)

    static float xyz[3] = {1.8, 0.0, 0.8};    // View position (x, y, z) [m]
    static float hpr[3] = {180.0, 0.0, 0.0};    // View direction （head,pitch,roll)

    dsSetViewpoint (xyz,hpr);                   // Set a view point
  	dsSetSphereQuality(3);
}


/***************************************************************************************************************
 ********************* FUNCTION FOR EACH LOOP: STEP FUNCTION ***************************************************
 ***************************************************************************************************************/

//"Pause" key pauses this loop, and any other key resumes it
static void simLoop (int pause)
{
    static int numSimSteps = 0;                 // Number of steps

    // control(): Assign the angle to the joint: target_angleR/L[i] to jointR/L[i]
    control();

    if (!pause)					                // If “pause” key is not pressed
    {
        dSpaceCollide(space,0,&nearCallback);	// Detect colision: nearCallback
        dWorldStep(world,SIM_STEP);             // Step of simulation
        dJointGroupEmpty(contactgroup);	        // Clear container of collision points
        numSimSteps++;                          // Increases number of steps
    }

    Model::rDrawRobot();				                // Draw robot with previously specified joint angles (in control() )

}



/***************************************************************************************************************
 ********************* FUNÇÃO PRINCIPAL ***************************************************************************
 ***************************************************************************************************************/

int main (int argc, char **argv)
{
    setDrawStuff();                         // Initialize Drawstuff "fn" parameters
    dInitODE();      				        // Initialize ODE

    world = dWorldCreate();          	    // Create a dynamic world
    space = dHashSpaceCreate(0);          	// Create a 3D space
    contactgroup = dJointGroupCreate(0);  	// Create a Joint group

    dWorldSetGravity(world,0,0,-9.8); 	    // Set gravity （x,y,z)
    dWorldSetERP (world, 0.95);	            // ERP: good: [0.8,1.0>
    dWorldSetCFM(world,1e-5);		        // CFM
    ground = dCreatePlane(space,0,0,1,0); 	// Create ground: plane (space,a,b,c,d)

    Model::rCreateRobot(world,space,jointR,jointL);             // Create robot

    dsSimulationLoop(argc,argv,600,400,&fn);// Simulation using Drawstuff "fn" parameters

    dJointGroupDestroy(contactgroup);	    // Destroy Joint group
    dSpaceDestroy(space); 			        // Destroy space
    dWorldDestroy (world);      		    // Destroy the world 　
    dCloseODE();                		    // Close ODE
    return 0;
}

/************** Accessory functions ****************************/
//Accessory function to start the drawstuff parameters
void setDrawStuff()
{   fn.version = DS_VERSION;     // The version
    fn.start   = &start;         // Start function
    fn.step    = &simLoop;       // Step function
    fn.command = NULL;           // Keyboard function (no keyboard input)
    fn.stop    = NULL;           // Stop function (no stop function)
    fn.path_to_textures = "./drawstuff/textures"; //　Path to the textures (.ppm)
}
