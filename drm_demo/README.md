# Accelize Distribution Platform
## Accelize FPGA DRM DEMO
---
This demo is intended to demonstrate the FPGA design protection and metering capability provided by the Accelize Distribution Platform.

The FPGA Design contains:
- The Accelize DRM IP
- A user IP embedding Accelize Activator IP 
The user ip reports its activation status throught register and generates periodic activity (usage units)

For more information about Accelize IPs and Accelize Reference Design, please refer to [Accelize Website](http://www.accelize.com/) and [Accelize GitHub](https://github.com/Accelize)



### Getting Started
#### Start VM
+ Amazon Web Services (AWS)
  + Create Instance using following parameters
     + Instance type : f1.16xlarge
     + AMI Public Image : Accelize_FPGA_DRM_Demo_v**
     
+ Nimbix
  + Login into [Nimbix Cloud](https://www.nimbix.net/)
  + Select "Compute" from right panel and search for "Accelize"
  + Run the "Accelize DRM Demo" application
  
#### Launch DRM Demo
+ VNC
Double-click on drmdemo icon on the Desktop

+ SSH
Open a terminal and run command "drmdemo"



### Demo features
+ Keyboard interface:
  + use LEFT, RIGHT arrow keys to select FPGA board
  + use ENTER, SPACE keys to start/stop application
  + use 'q' key to quit demo
+ Mouse interface:
  + click on a column to select FPGA board
  + click on an highlighted column to start application
  + click on "ACCELIZE DRM DEMO" title to quit the demo


### Run with your credentials
+ Create an account on [Accelie DRM Portal](https://drmportal.accelize.com)
+ Select "END USERS" > "ACCESS KEYS" menu and create your access key
+ Edit /opt/accelize/drm_demo/conf/user/cred.json with with access key
+ run the application using your account email:
```
/opt/accelize/drm_demo/run.sh -u <your_email@your_company.com>
```

### Monitor the application usage units
+ Connect on [Accelie DRM Portal](https://drmportal.accelize.com)
+ Select "END USERS" > "USAGE" menu
