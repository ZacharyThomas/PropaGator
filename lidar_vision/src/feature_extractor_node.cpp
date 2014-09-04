#include <ros/ros.h>
#include <sensor_msgs/PointCloud2.h>
#include <pcl/point_cloud.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl/sample_consensus/sac_model_circle3d.h>			//Use 2D if you squash the data to two dimensions
#include <pcl/sample_consensus/ransac.h>

/*** TODO:
 * 		Pick the best robust sample consensus estimator
 ***/

class FeatureExtractor
{
private:	//Typedefs
	typedef pcl::PointXYZ point;

private:	//Vars
	ros::Subscriber pc_sub_;					//Subscriber to segmented point cloud data
	ros::Publisher buoy_pub_, shore_pub_;		//publisher for buoy's and land masses respectively
	double distance_threshold_;					//... TODO: figure out exactly how this effects the computation
	int max_iterations_;						//Maximum number of itterations before giving up
	double probability_;						//... TODO: figure out exactly how this effects the computation
	double max_buoy_radius_;					//The maximum size that we consider a buoy

private:	//Functions
	void extract(const sensor_msgs::PointCloud2::ConstPtr& pc_msg)
	{
		ROS_INFO("Begining extraction");
		//Convert from msgs to PointCloud<point>
		pcl::PointCloud<point>::Ptr pc_in(new pcl::PointCloud<point>);
		pcl::fromROSMsg(*pc_msg, *pc_in);

		//Vector of ints to hold the indices of the inliers
		std::vector<int> inliers;

		//Create a Random sample consensus model
		pcl::SampleConsensusModelCircle3D<point>::Ptr
			circle(new pcl::SampleConsensusModelCircle3D<point>(pc_in));

		//Create a random sample consensus object
		//We use the RANSAC (RANdom SAmple consensus) estimator mainly since that is what is
		//Suggested if you don't know how the others work see below link for more info
		//http://docs.pointclouds.org/trunk/group__sample__consensus.html
		pcl::RandomSampleConsensus<point> ransac(circle);

		//Set up
		ransac.setDistanceThreshold(distance_threshold_);
		ransac.setMaxIterations(max_iterations_);
		ransac.setProbability(probability_);

		//Perform calculations
		ransac.computeModel();
		ransac.getInliers(inliers);
		if(static_cast<int>(inliers.size()) == 0)
		{
			return;
		}
		ROS_INFO("Size of inliers is %i", static_cast<int>(inliers.size()));

		//Determine if this is good enough to be a buoy
		Eigen::VectorXf coeff;														//Vector to hold coeff
		ransac.getModelCoefficients(coeff);											//Get model coefficients
		//ROS_INFO("Model Params are x:%f y:%f z:%f radius:%f nx:%f ny:%f nz:%f",
		//		coeff[0], coeff[1], coeff[2], coeff[3], coeff[4], coeff[5], coeff[6]);
		//Index 3 is radius as defined in http://docs.pointclouds.org/trunk/group__sample__consensus.html

		//Publish to correct topic
		sensor_msgs::PointCloud2::Ptr out_msg(new sensor_msgs::PointCloud2);		//Make output msg
		pcl::PointCloud<point>::Ptr out_pc(new pcl::PointCloud<point>);				//Make out point cloud
		if(coeff[3] < max_buoy_radius_)
		{
			pcl::copyPointCloud(*pc_in, inliers, *out_pc);							//Copy inliers from out_pc
			pcl::toROSMsg(*out_pc, *out_msg);										//Generate msg
			buoy_pub_.publish(out_msg);												//Publish a buoy
		}


	}

public:		//Functions
	//Constructor
	FeatureExtractor()
	{
		ros::NodeHandle private_nh("~");

		std::string topic;

		//Get some ros params
		topic = private_nh.resolveName("distance_threshold");			//Distance threshold
		private_nh.param<double>(topic.c_str(), distance_threshold_, 1.0);
		ROS_INFO("Param %s value %f", topic.c_str(), distance_threshold_);

		topic = private_nh.resolveName("max_iterations");				//Max iterations
		private_nh.param<int>(topic.c_str(), max_iterations_, 1000);
		ROS_INFO("Param %s value %i", topic.c_str(), max_iterations_);

		topic = private_nh.resolveName("probability");					//Probability
		private_nh.param<double>(topic.c_str(), probability_, 0.99);
		ROS_INFO("Param %s value %f", topic.c_str(), probability_);

		topic = private_nh.resolveName("max_buoy_radius");					//Probability
		private_nh.param<double>(topic.c_str(), max_buoy_radius_, 0.5);
		ROS_INFO("Param %s value %f", topic.c_str(), max_buoy_radius_);

		ros::NodeHandle public_nh;
		//Set up subscribers and publishers
		topic = public_nh.resolveName("segmented_pc");		//Get the topic
		pc_sub_ = public_nh.subscribe<sensor_msgs::PointCloud2>(topic.c_str(), 100, &FeatureExtractor::extract, this);
		buoy_pub_ = public_nh.advertise<sensor_msgs::PointCloud2>("buoy", 100);
	}
};

/************************
 * 			Main		*
 ************************/
int main(int argc, char** argv)
{
	//Initialize ROS
	ros::init(argc, argv, "feature_extractor_node");
	ros::NodeHandle nh;
	ros::Rate update_rate(100);

	//Set up the extractor
	FeatureExtractor extractor;

	while(ros::ok())
	{
		update_rate.sleep();
		ros::spinOnce();
	}
	return 0;
}
