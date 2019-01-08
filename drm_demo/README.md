# Accelize Distribution Platform
![Accelize Logo](image/accelize_logo.png)
## Accelize FPGA DRM DEMO
---
This demo is intended to demonstrate the FPGA design protection and metering capability provided by the Accelize Distribution Platform.

It uses [Nimbix Cloud](https://www.nimbix.net/) FPGA instance with Xilinx Alveo board.
The FPGA Design contains:
- The Accelize DRM IP 
- A user IP embedding Accelize Activator IP 
The user ip reports its activation status throught register and generating periodic activity (usage units)

![Accelize Reference Design](image/accelize_refdesign.png)

For more information about Accelize IPs and Accelize Reference Design, please refer to [Accelize Website](http://www.accelize.com/) and [Accelize GitHub](https://github.com/Accelize)

## How to use the Demo
### Prerequisites
You need to create an account on [Nimbix Cloud](https://www.nimbix.net/).
You can have a 100 hours free trial use [this form](https://www.nimbix.net/alveotrial/)

### Getting Started
+ Login into [Nimbix Cloud](https://www.nimbix.net/)
+ Select "Compute" from right panel and search for "Accelize"
+ Run the "Accelize" application
+ Open a terminal and run command
```
drmdemo
```

### Demo features
+ To be completed
