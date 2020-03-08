#include "nuslam/ekf.hpp"

namespace nuslam
{
	// Used to for prediction stage
    using rigid2d::Twist2D;

    // used for model update
    using rigid2d::Pose2D;

    // used for map update
    using rigid2d::Vector2D;

	// CovarianceMatrix
	CovarianceMatrix::CovarianceMatrix()
	{
        std::vector<double> robot_state_cov{0.0, 0.0, 0.0}; // init to 0,0,0
        // 3*3
        // construct EigenVector from Vector
        Eigen::VectorXd robot_state_cov_vct = Eigen::VectorXd::Map(robot_state_cov.data(), robot_state_cov.size());
        Eigen::MatrixXd robot_cov_mtx = robot_state_cov_vct.asDiagonal();

        cov_mtx = robot_cov_mtx;
	}

	CovarianceMatrix::CovarianceMatrix(const std::vector<Vector2D> & map_state_)
	{
        std::vector<double> robot_state_cov{0.0, 0.0, 0.0}; // init to 0,0,0
        // 3*3
        // construct EigenVector from Vector
        Eigen::VectorXd robot_state_cov_vct = Eigen::VectorXd::Map(robot_state_cov.data(), robot_state_cov.size());
        Eigen::MatrixXd robot_cov_mtx = robot_state_cov_vct.asDiagonal();

        map_state = map_state_;
        // 2n*2n diag matrix since map containts x1,y1, ... xn,yn
        std::vector<double> map_state_cov(2 * map_state.size(), std::numeric_limits<double>::infinity()); // init to 0,0,0
        // construct EigenVector from Vector
        Eigen::VectorXd map_state_cov_vct = Eigen::VectorXd::Map(map_state_cov.data(), map_state_cov.size());
        Eigen::MatrixXd map_cov_mtx = map_state_cov_vct.asDiagonal();

        // 2n*3
        Eigen::MatrixXd bottom_left = Eigen::MatrixXd::Zero(2 * map_state.size(), 3);
        // 3*2n
        Eigen::MatrixXd top_right = Eigen::MatrixXd::Zero(3, 2 * map_state.size());

        // Construct left half of matrix
        Eigen::MatrixXd left_mtx(robot_cov_mtx.rows() + bottom_left.rows(), robot_cov_mtx.cols());
        left_mtx << robot_cov_mtx, bottom_left;
        // Construct right half of matrix
        Eigen::MatrixXd right_mtx(top_right.rows() + map_cov_mtx.rows(), top_right.cols());
        right_mtx << top_right, map_cov_mtx;

        // Merge two matrixes to construct Covariance Matrix
        Eigen::MatrixXd mtx(left_mtx.rows(), left_mtx.cols() + right_mtx.cols());
        mtx << left_mtx, right_mtx;

        cov_mtx = mtx;
	}

	CovarianceMatrix::CovarianceMatrix(const std::vector<Vector2D> & map_state_, \
                         			   const std::vector<double> & robot_state_cov_,\
			                           const std::vector<double> & map_state_cov_)
	{
        robot_state_cov = robot_state_cov_;
        // construct EigenVector from Vector
        Eigen::VectorXd robot_state_cov_vct = Eigen::VectorXd::Map(robot_state_cov.data(), robot_state_cov.size());
        Eigen::MatrixXd robot_cov_mtx = robot_state_cov_vct.asDiagonal();

        map_state = map_state_;
        map_state_cov = map_state_cov_;
        // construct EigenVector from Vector
        Eigen::VectorXd map_state_cov_vct = Eigen::VectorXd::Map(map_state_cov.data(), map_state_cov.size());
        Eigen::MatrixXd map_cov_mtx = map_state_cov_vct.asDiagonal();

        // 2n*3
        Eigen::MatrixXd bottom_left = Eigen::MatrixXd::Zero(2 * map_state.size(), 3);
        // 3*2n
        Eigen::MatrixXd top_right = Eigen::MatrixXd::Zero(3, 2 * map_state.size());

        // Construct left half of matrix
        Eigen::MatrixXd left_mtx(robot_cov_mtx.rows() + bottom_left.rows(), robot_cov_mtx.cols());
        left_mtx << robot_cov_mtx, bottom_left;
        // Construct right half of matrix
        Eigen::MatrixXd right_mtx(top_right.rows() + map_cov_mtx.rows(), top_right.cols());
        right_mtx << top_right, map_cov_mtx;

        // Merge two matrixes to construct Covariance Matrix
        Eigen::MatrixXd mtx(left_mtx.rows(), left_mtx.cols() + right_mtx.cols());
        mtx << left_mtx, right_mtx;

        cov_mtx = mtx;
	}

	// Process Noise
	ProcessNoise::ProcessNoise()
	{
		cov_mtx = CovarianceMatrix();

		xyt_noise = Pose2D();

		Eigen::VectorXd xyt_vct(3);

		xyt_vct << xyt_noise.x, xyt_noise.y, xyt_noise.theta;

		// assume x,y,theta noise decoupled from each other, 3*3
		Eigen::MatrixXd q = xyt_vct.asDiagonal();

		Eigen::MatrixXd Q = q;

	}

	ProcessNoise::ProcessNoise(const Pose2D & xyt_noise_var, const CovarianceMatrix & cov_mtx_)
	{
		cov_mtx = cov_mtx_;
		// std::vector<double> noise_vect = get_3d_noise(xyt_noise_mean, xyt_noise_var, cov_mtx);
		xyt_noise = Pose2D(xyt_noise_var.x, xyt_noise_var.y, xyt_noise_var.theta);

		Eigen::VectorXd xyt_vct(3);

		xyt_vct << xyt_noise.x, xyt_noise.y, xyt_noise.theta;

		// assume x,y,theta noise decoupled from each other, 3*3
		Eigen::MatrixXd q = xyt_vct.asDiagonal();

		// 2n*3
        Eigen::MatrixXd bottom_left = Eigen::MatrixXd::Zero(2 * cov_mtx.map_state_cov.size(), 3);
        // 3*2n
        Eigen::MatrixXd top_right = Eigen::MatrixXd::Zero(3, 2 * cov_mtx.map_state_cov.size());
        // 2*2n
        Eigen::MatrixXd bottom_right = Eigen::MatrixXd::Zero(2 * cov_mtx.map_state_cov.size(), 2 * cov_mtx.map_state_cov.size());

        // Construct left half of matrix
        Eigen::MatrixXd left_mtx(q.rows() + bottom_left.rows(), q.cols());
        left_mtx << q, bottom_left;
        // Construct right half of matrix
        Eigen::MatrixXd right_mtx(top_right.rows() + bottom_right.rows(), top_right.cols());
        right_mtx << top_right, bottom_right;

        // Merge two matrixes to construct Covariance Matrix
        Eigen::MatrixXd mtx(left_mtx.rows(), left_mtx.cols() + right_mtx.cols());
        mtx << left_mtx, right_mtx;

        Q = mtx;

	}


	// Random Sampling Functions
	std::mt19937 & get_random()
    {
        // static variables inside a function are created once and persist for the remainder of the program
        static std::random_device rd{}; 
        static std::mt19937 mt{rd()};
        // we return a reference to the pseudo-random number genrator object. This is always the
        // same object every time get_random is called
        return mt;
    }

    double sampleNormalDistribution()
    {
		std::normal_distribution<> d(0, 1);
		return d(get_random());
    }

    std::vector<double> get_3d_noise(const Pose2D & xyt_noise_mean, const ProcessNoise & proc_noise)
    {
    	Eigen::MatrixXd Sigma = proc_noise.q;

    	// Cholesky Decomposition
    	Eigen::MatrixXd L(Sigma.llt().matrixL());

    	// Sample Random Noise
    	// x
    	const double x_normal = sampleNormalDistribution();
    	// y
    	const double y_normal = sampleNormalDistribution();
    	// theta
    	const double theta_normal = sampleNormalDistribution();

    	// Get noise from 3D normal distribution and store in vector for each individual dimension
    	// mean noise is generally zero
    	const auto x_noise = xyt_noise_mean.x + L(0,0) * x_normal + L(0,1) * y_normal + L(0,2) * theta_normal;
		const auto y_noise = xyt_noise_mean.y + L(1,0) * x_normal + L(1,1) * y_normal + L(1,2) * theta_normal;
		const auto theta_noise = xyt_noise_mean.theta + L(2,0) * x_normal + L(2,1) * y_normal + L(2,2) * theta_normal;

		std::vector<double> noise_vect;

		noise_vect.push_back(x_noise);
		noise_vect.push_back(y_noise);
		noise_vect.push_back(theta_noise);

		return noise_vect;
    }

    //EKF
    EKF::EKF()
    {
    	robot_state = Pose2D();
    	proc_noise = ProcessNoise();
    	cov_mtx = CovarianceMatrix();
    }

    EKF::EKF(const Pose2D & robot_state_, const std::vector<Vector2D> & map_state_, const Pose2D & xyt_noise_var)
    {
    	robot_state = robot_state_;
    	map_state = map_state_;
    	cov_mtx = CovarianceMatrix(map_state_);
    	proc_noise = ProcessNoise(xyt_noise_var, cov_mtx);
    }

    void EKF::predict(const Twist2D & twist, const Pose2D & xyt_noise_mean)
    {
    	// First, update the estimate using the forward model
    	std::vector<double> noise_vect = get_3d_noise(xyt_noise_mean, proc_noise);

    	Pose2D belief;

    	if (rigid2d::almost_equal(twist.w_z, 0.0))
    	// If dtheta = 0
    	{
    		belief = Pose2D(robot_state.x + (twist.v_x * cos(robot_state.theta)) + noise_vect.at(0),\
						    robot_state.y + (twist.v_x * sin(robot_state.theta)) + noise_vect.at(1),\
    						robot_state.theta + noise_vect.at(2));
    	} else {
		// If dtheta != 0
    		belief = Pose2D(robot_state.x + ((-twist.v_x / twist.w_z) * sin(robot_state.theta) + (twist.v_x / twist.w_z) * sin(robot_state.theta + twist.w_z)) + noise_vect.at(0),\
						    robot_state.y + ((twist.v_x / twist.w_z) * cos(robot_state.theta) + (-twist.v_x / twist.w_z) * cos(robot_state.theta + twist.w_z)) + noise_vect.at(1),\
    						robot_state.theta + twist.w_z + noise_vect.at(2));

    	}

    	// Next, we propagate the uncertainty using the linearized state transition model
    	Eigen::MatrixXd G;
    	if (rigid2d::almost_equal(twist.w_z, 0.0))
    	// If dtheta = 0
    	{
    		Eigen::MatrixXd g = Eigen::MatrixXd::Zero(3 + (2 * map_state.size()), 3 + (2 * map_state.size()));
    		// Now replace non-zero entries
    		g(1, 0) = -twist.v_x * sin(robot_state.theta);
    		g(2, 0) = twist.v_x * cos(robot_state.theta);

    		G = Eigen::MatrixXd::Identity(3 + (2 * map_state.size()), 3 + (2 * map_state.size())) + g;
    	} else {
		// If dtheta != 0
    		Eigen::MatrixXd g = Eigen::MatrixXd::Zero(3 + (2 * map_state.size()), 3 + (2 * map_state.size()));
    		// Now replace non-zero entries
    		g(1, 0) = (-twist.v_x / twist.w_z) * cos(robot_state.theta) + (twist.v_x / twist.w_z) * cos(robot_state.theta + twist.w_z);
    		g(2, 0) = (-twist.v_x / twist.w_z) * sin(robot_state.theta) + (twist.v_x / twist.w_z) * sin(robot_state.theta + twist.w_z);

    		G = Eigen::MatrixXd::Identity(3 + (2 * map_state.size()), 3 + (2 * map_state.size())) + g;
    	}

    	cov_mtx.cov_mtx = G * cov_mtx.cov_mtx * G.transpose() + proc_noise.Q;
    }

    // EKF::update()
    // {

    // }
}