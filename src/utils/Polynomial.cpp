#include "Polynomial.h"
#include <Arduino.h>

Polynomial::Polynomial() : degree_(1) {
    // Default: identity function y = x
    coefficients_[0] = 0.0f;  // a0
    coefficients_[1] = 1.0f;  // a1
    for (int i = 2; i <= MAX_DEGREE; i++) {
        coefficients_[i] = 0.0f;
    }
}

Polynomial::Polynomial(const float* coeffs, int degree) : degree_(degree) {
    if (degree > MAX_DEGREE) {
        Serial.printf("Warning: Polynomial degree %d exceeds MAX_DEGREE %d, clamping\n", 
                     degree, MAX_DEGREE);
        degree_ = MAX_DEGREE;
    }
    
    if (degree < 0) {
        Serial.println("Warning: Polynomial degree < 0, defaulting to identity");
        degree_ = 1;
        coefficients_[0] = 0.0f;
        coefficients_[1] = 1.0f;
        return;
    }
    
    // Copy coefficients
    for (int i = 0; i <= degree_; i++) {
        coefficients_[i] = coeffs[i];
    }
    
    // Zero out unused coefficients
    for (int i = degree_ + 1; i <= MAX_DEGREE; i++) {
        coefficients_[i] = 0.0f;
    }
}

float Polynomial::evaluate(float x) const {
    // Horner's method for efficient polynomial evaluation
    // For polynomial a0 + a1*x + a2*x^2 + a3*x^3:
    // = a0 + x*(a1 + x*(a2 + x*a3))
    
    float result = coefficients_[degree_];
    for (int i = degree_ - 1; i >= 0; i--) {
        result = result * x + coefficients_[i];
    }
    return result;
}

void Polynomial::setCoefficients(const float* coeffs, int degree) {
    if (degree > MAX_DEGREE) {
        Serial.printf("Warning: Polynomial degree %d exceeds MAX_DEGREE %d, clamping\n", 
                     degree, MAX_DEGREE);
        degree = MAX_DEGREE;
    }
    
    if (degree < 0) {
        Serial.println("Warning: Polynomial degree < 0, ignoring update");
        return;
    }
    
    degree_ = degree;
    
    for (int i = 0; i <= degree_; i++) {
        coefficients_[i] = coeffs[i];
    }
    
    for (int i = degree_ + 1; i <= MAX_DEGREE; i++) {
        coefficients_[i] = 0.0f;
    }
}

float Polynomial::getCoefficient(int i) const {
    if (i < 0 || i > MAX_DEGREE) {
        return 0.0f;
    }
    return coefficients_[i];
}
