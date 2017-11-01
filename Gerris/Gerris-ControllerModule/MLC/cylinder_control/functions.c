static double actuator_degrees(double x, double y) {
    return atan2(y, x);
}

static double gaussian_curve(float degrees) {
    static const float mu = ACT_THETA_CENTER;
    static const float sigma = ACT_SIGMA_COEF * (ACT_THETA_WIDTH / 2.0);
    if (degrees < 0)
        degrees = -degrees;
    float aux = (degrees - mu) / sigma;
    return 1 * exp(-0.5 * aux * aux);
}

static double degrees_in_range(float degrees){
    static const float degrees_min = ACT_THETA_CENTER - ACT_THETA_WIDTH / 2.0;
    static const float degrees_max = ACT_THETA_CENTER + ACT_THETA_WIDTH / 2.0;

    if (degrees < 0)
        degrees = -degrees;
    return degrees >= degrees_min && degrees <= degrees_max;
}

static double actuation_u(double x, double y) {
    double degrees = actuator_degrees(x, y);
    if (degrees_in_range(degrees)) {
        float act = gaussian_curve(degrees) * controller("actuation");
        return (degrees > 0) ? y*act : -y*act;
    }
    else
        return 0;
}
static double actuation_v(double x, double y) {
    double degrees = actuator_degrees(x, y);
    if (degrees_in_range(degrees)) {
        float act = gaussian_curve(degrees) * controller("actuation");
        return (degrees > 0) ? -x*act : x*act;
    }
    else
        return 0;
}
static int symmetric_refinement(double x, double y) {
    return (1 - fabs(2*y/BOX_SIZE)) * (REFINE_MAX - REFINE_MIN) + REFINE_MIN;
}
static int positive_quad_symmetric_refinement(double x, double y) {
    double yAbs = fabs(y);
    return (REFINE_MAX-REFINE_MIN)*(2*yAbs/BOX_SIZE - 1)*(2*yAbs/BOX_SIZE - 1) + REFINE_MIN;
}
static int negative_quad_symmetric_refinement(double x, double y) {
    return -(REFINE_MAX-REFINE_MIN)*(2*y/BOX_SIZE)*(2*y/BOX_SIZE) + REFINE_MAX;
}

