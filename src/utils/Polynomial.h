#ifndef POLYNOMIAL_H
#define POLYNOMIAL_H

class Polynomial {
public:
    Polynomial(const float* coeffs, int degree);
    Polynomial();
    
    float evaluate(float x) const;
    
    void setCoefficients(const float* coeffs, int degree);
    
    int getDegree() const { return degree_; }
    
    float getCoefficient(int i) const;

private:
    static constexpr int MAX_DEGREE = 5;
    float coefficients_[MAX_DEGREE + 1];
    int degree_;
};

#endif
