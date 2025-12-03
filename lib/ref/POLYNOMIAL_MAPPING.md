# Polynomial Velocity Mapping

## Overview
Replaced the linear feedforward model (`PWM = deadzone + gain × velocity`) with cubic polynomial regression for accurate motor control across the full speed range.

## What Was Changed

### New Files
- **`src/Polynomial.h`** - Generic polynomial evaluator (supports up to 5th degree)
- **`src/Polynomial.cpp`** - Efficient Horner's method evaluation

### Modified Files
- **`src/VelocityController.h/.cpp`** 
  - Added `pwmToVelocityPoly` and `velocityToPWMPoly` members
  - Modified `velocityToPWM()` to use polynomial when enabled
  - Added functions: `setPWMToVelocityPolynomial()`, `setVelocityToPWMPolynomial()`, `enablePolynomialMapping()`

- **`src/WebServer.cpp`**
  - Added `POLY_VEL2PWM:degree,a0,a1,a2,...` command
  - Added `POLY_PWM2VEL:degree,b0,b1,b2,...` command  
  - Added `POLY_ENABLE:true/false` command

- **`data/calibration.html`**
  - Added "Polynomial Velocity Mapping" panel with coefficient inputs
  - Added functions: `sendVel2PWMPolynomial()`, `sendPWM2VelPolynomial()`, `togglePolynomialMapping()`

## How to Use

### Step 1: Collect Calibration Data
1. Open the **Calibration** page
2. Go to **Automatic Voltage Sweep Calibration**
3. Configure:
   - Start PWM: 60 (or your deadzone)
   - End PWM: 240 (avoid saturation)
   - Step Size: 5
   - Hold Time: 2000 ms (2 seconds per step)
4. Click **Start Left Motor** (or Right Motor)
5. Export the CSV when complete

### Step 2: Fit Cubic Polynomial
Use Python (NumPy) or MATLAB to fit a 3rd-degree polynomial:

#### Python Example:
```python
import numpy as np
import pandas as pd

# Load your CSV
df = pd.read_csv('motor_calibration.csv')
pwm = df['PWM'].values
velocity = df['Velocity'].values  # cm/s

# Fit cubic polynomial: velocity = b0 + b1*PWM + b2*PWM^2 + b3*PWM^3
pwm_to_vel_coeffs = np.polyfit(pwm, velocity, 3)  # Returns [b3, b2, b1, b0]
print("PWM->Velocity coefficients (b3, b2, b1, b0):", pwm_to_vel_coeffs)

# Fit inverse: PWM = a0 + a1*vel + a2*vel^2 + a3*vel^3
vel_to_pwm_coeffs = np.polyfit(velocity, pwm, 3)  # Returns [a3, a2, a1, a0]
print("Velocity->PWM coefficients (a3, a2, a1, a0):", vel_to_pwm_coeffs)
```

#### MATLAB/Octave:
```matlab
pwm_to_vel_coeffs = polyfit(PWM, Velocity, 3);  % [b3, b2, b1, b0]
vel_to_pwm_coeffs = polyfit(Velocity, PWM, 3);  % [a3, a2, a1, a0]
```

**Note:** NumPy/MATLAB return coefficients in **descending** order: `[a3, a2, a1, a0]`

### Step 3: Enter Coefficients in UI
1. Open **Calibration → Polynomial Velocity Mapping**
2. In **Velocity → PWM Control** panel:
   - a₀ = vel_to_pwm_coeffs[3] (constant term)
   - a₁ = vel_to_pwm_coeffs[2] (linear)
   - a₂ = vel_to_pwm_coeffs[1] (quadratic)
   - a₃ = vel_to_pwm_coeffs[0] (cubic)
   - Click **Upload to Robot**

3. In **PWM → Velocity (Forward Model)** panel:
   - b₀ = pwm_to_vel_coeffs[3]
   - b₁ = pwm_to_vel_coeffs[2]
   - b₂ = pwm_to_vel_coeffs[1]
   - b₃ = pwm_to_vel_coeffs[0]
   - Click **Upload to Robot**

4. **Check the box** "Enable Polynomial Mapping"

### Step 4: Verify
1. Use **Manual Velocity Feedforward Tuning** section
2. Set target velocities: 10, 20, 30, 40 cm/s
3. Compare measured velocity to target
4. Velocity tracking should now be accurate across the full range!

## Why This Works
- **Linear model**: Assumes constant relationship (fails at low/high speeds due to friction, back-EMF)
- **Cubic polynomial**: Captures non-linear motor dynamics (friction transition, saturation effects)
- **Two polynomials**: 
  - **Velocity→PWM**: Used for control (what PWM to command for desired velocity)
  - **PWM→Velocity**: Used for feedforward prediction/diagnostics

## Fallback Mode
If you uncheck "Enable Polynomial Mapping", the system reverts to the legacy linear feedforward model (`deadzone + gain × velocity`). This is useful for:
- Comparison testing
- Simple tuning when polynomial coefficients aren't available
- Debugging

## Example Coefficients (Typical)
These are **examples only** - you must calibrate your specific robot:

### Velocity → PWM (for control)
```
a₀ = 55.2     (base PWM offset)
a₁ = 2.8      (dominant linear term)
a₂ = 0.015    (quadratic correction)
a₃ = 0.00025  (cubic fine-tuning)
```

### PWM → Velocity (forward model)
```
b₀ = -18.5    (velocity offset at zero PWM)
b₁ = 0.42     (linear response)
b₂ = 0.0012   (quadratic term)
b₃ = 0.000003 (cubic term)
```

## Troubleshooting

**"Velocity tracking still off"**
- Check you entered coefficients in correct order (a₀, a₁, a₂, a₃)
- Verify polynomial is enabled (checkbox)
- Re-run calibration sweep with more PWM points

**"Robot doesn't move at low velocities"**
- Polynomial may predict PWM below motor deadzone
- Check a₀ coefficient - should be near your deadzone value

**"Polynomial gives negative PWM"**
- Check coefficient signs and fit quality (R² > 0.95 recommended)
- Ensure calibration data covered full range (60-240 PWM)

## Next Steps
After polynomial mapping is working:
1. Re-run **PID Autotuner** with polynomial enabled
2. The autotuner will now tune PID around accurate feedforward
3. Result: tight velocity tracking with minimal steady-state error

---
**Note:** This system keeps the legacy linear feedforward as a fallback. You can toggle between modes at runtime without reflashing firmware.
