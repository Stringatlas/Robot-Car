#ifndef POLYNOMIAL_H
#define POLYNOMIAL_H

/**
 * Generic polynomial evaluator for arbitrary degree polynomials.
 * Supports both forward (PWM->velocity) and inverse (velocity->PWM) mappings.
 */
class Polynomial {
public:
    /**
     * Construct a polynomial with given coefficients.
     * @param coeffs Array of coefficients [a0, a1, a2, ..., an] for polynomial:
     *               y = a0 + a1*x + a2*x^2 + ... + an*x^n
     * @param degree Degree of polynomial (number of coefficients - 1)
     */
    Polynomial(const float* coeffs, int degree);
    
    /**
     * Default constructor - creates identity polynomial (y = x)
     */
    Polynomial();
    
    /**
     * Evaluate polynomial at given x value
     * @param x Input value
     * @return Output value y = f(x)
     */
    float evaluate(float x) const;
    
    /**
     * Update polynomial coefficients
     * @param coeffs New coefficient array
     * @param degree New degree
     */
    void setCoefficients(const float* coeffs, int degree);
    
    /**
     * Get polynomial degree
     */
    int getDegree() const { return degree_; }
    
    /**
     * Get coefficient at index i
     */
    float getCoefficient(int i) const;

private:
    static constexpr int MAX_DEGREE = 5;  // Support up to 5th degree polynomials
    float coefficients_[MAX_DEGREE + 1];
    int degree_;
};

#endif // POLYNOMIAL_H
