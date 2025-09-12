// for using fopen
#include <stdio.h> 
// for testing
#include <iostream>
// for signal management
#include <signal.h>
#include <cmath>
#include <unistd.h>

void filecheck(FILE *checkme, std::string fLocation){

    // checks if file is empty
    if (checkme == NULL){
        
        std::clog << "Error Opening " << fLocation << ", does it exist?" << std::endl;
        exit(1);

    }

}

void sighandler(int s){
    
    std::clog << "Exited with code " << s << "\n"; // output signal to log
    exit(1);

}

// takes Upper limit, Lower limit, the "k"urviture, and midpoint
// f(x) = ((U-L)/(1+e^(-k*(x-xsub0)))+L
int sigmoid(int x, int upperLimit, int lowerLimit, float kurve, int midpoint){
    
    return (((upperLimit - lowerLimit) / (1+(exp(-1*(kurve)*(x-midpoint)))))+lowerLimit);
    
}  

int getDieTemp(std::string temp5){

    // opens file to read temps temp5 is TC0C, ie the die in applese
    FILE *f = fopen(temp5.c_str(), "r");
    filecheck(f, temp5); // checks if file is valid

    // creating variable(s) to pass to fgets()
    char tempstring[6];

    // reads the entire line, there's only one lol
    fgets(tempstring, 6, f);

    // cleanup
    fclose(f);

    return (atoi(tempstring)/1000);
}

void setSpeed(int nSpeed, std::string speedPath, int fanNum){

    // run the loop here to prevent fan speed race hazard
    for (int i = 1; i <= fanNum; i++){
        // opens file for reading and writing without making a new one, I think
        FILE *fSpeed = fopen((speedPath + "fan" + std::to_string(i) + "_min").c_str(), "r+");
        filecheck(fSpeed, (speedPath + "fan" + std::to_string(i) + "_min"));

        // converts int to char array ig
        //std::string strSpeed = std::to_string(nSpeed);
        fputs(std::to_string(nSpeed).c_str(), fSpeed);

        // cleanup
        fclose(fSpeed);
    }

}

void readConfig(int *fanNum, int *minSpeed, int *maxSpeed, float *kurve, int *midpoint, std::string configPath){

    FILE *fConfig = fopen(configPath.c_str(), "r");
    if (fConfig == NULL){
        std::clog << "Please configure Curby with -c";
        exit(1);
    }

    // this whole thing searches for a needle (vars) in a haystack (configuration file)
    // and updates the variables according to what it found.
    char haystack[64];
    std::string needle;
    while ((fgets(haystack, sizeof(haystack), fConfig) != NULL)){

        if(haystack[0] == '/n' or haystack[0] == '#'){
            continue;
        }

        // searches for the value in the substring after the delimiter and removes /n        
        if(std::string(haystack).find("fanNum")   != std::string::npos){

            needle = (std::string(haystack).substr(std::string(haystack).find("=")+1));
            needle.pop_back();
            *fanNum = stoi(needle);
            std::clog << "fanNum = " << *fanNum << std::endl;

        }

        if(std::string(haystack).find("minSpeed") != std::string::npos){

            needle = (std::string(haystack).substr(std::string(haystack).find("=")+1));
            needle.pop_back();
            *minSpeed = stoi(needle);
            std::clog << "minspeed = " << *minSpeed << std::endl;

        }
        
        if(std::string(haystack).find("maxSpeed") != std::string::npos){

            needle = (std::string(haystack).substr(std::string(haystack).find("=")+1));
            needle.pop_back();
            *maxSpeed = stoi(needle);
            std::clog << "maxSpeed = " << *maxSpeed << std::endl;
        
        }
        
        if(std::string(haystack).find("kurve")    != std::string::npos){
        
            needle = (std::string(haystack).substr(std::string(haystack).find("=")+1));
            needle.pop_back();
            *kurve = stof(needle);
            std::clog << "kurve = " << *kurve << std::endl;
        
        }
        
        if(std::string(haystack).find("midpoint") != std::string::npos){
        
            needle = (std::string(haystack).substr(std::string(haystack).find("=")+1));
            needle.pop_back();
            *midpoint = stoi(needle);
            std::clog << "midpoint = " << *midpoint << std::endl;
        
        }

    }

    // cleanup
    fclose(fConfig);
}

void configure(std::string configPath, float k, int m){

    std::string uInput;
    FILE *fConfig = fopen(configPath.c_str(), "w");

    // each block gets user input and outputs it to the filestream
    std::cout << "Number of fans: (Default is 1)\n";
    getline(std::cin, uInput);
    if (uInput.empty()){
        uInput = "1";
    }
    fputs(("fanNum="    + uInput + "\n").c_str(), fConfig);

    std::cout << "Fans MINIMUM recommended speed: (Default is 2300)\n";
    getline(std::cin, uInput);
    if (uInput.empty()){
        uInput = "2300";
    }
    fputs(("minSpeed="  + uInput + "\n").c_str(), fConfig);

    std::cout << "Fans MAXIMUM recommended speed: (Default is 5500)\n";
    getline(std::cin, uInput);
    if (uInput.empty()){
        uInput = "5500";
    }
    fputs(("maxSpeed="  + uInput + "\n").c_str(), fConfig);
    
    std::cout << "You can fine tune the curve and adjust other settings in:\n" << configPath << "\n";
    fputs(("kurve="     + std::to_string(k) + "\n").c_str(), fConfig);
    fputs(("midpoint="  + std::to_string(m) + "\n").c_str(), fConfig);

    fclose(fConfig);
    exit(0);
}

int main(int argc, char* argv[]) {

    if(geteuid() != 0){

        std::cout << "Run Curby as root" << std::endl;
        exit(0);

    }

    // sets up signal handlers incase of process death via outside means
    signal(SIGINT, sighandler);  // sigint ie ctrl + c
    signal(SIGTERM, sighandler); // sigterm ie systemd stop or pkill

    // arguments
    int opt;
    // function kurvy-ness
    float kurve = 0.1;
    // function midpoint location;
    int midpoint = 55;
    // Lower Limit in RPM
    int minSpeed = 2300;
    // Upper Limit in RPM
    int maxSpeed = 5500;
    // number of fans in the system
    int fanNum = 1;
    // DriverPath
    const std::string dPath = "/sys/devices/platform/applesmc.768/";
    // config location
    const std::string cPath = "/etc/curby/curby.conf";

    // argument handler
    while((opt = getopt(argc, argv, "if:c")) != -1) {
    
        switch(opt) {
            case 'c':

                configure(cPath, kurve, midpoint);

            default:
                break;
            
        }
    }

    readConfig(&fanNum, &minSpeed, &maxSpeed, &kurve, &midpoint, cPath);

    // read temp1_input for die temp
    while (true){
    	
        // based on a sigmoid function where x = the die temp set the speed of the fans
        setSpeed(sigmoid(getDieTemp(dPath+"temp5_input"), maxSpeed, minSpeed, kurve, midpoint), dPath, fanNum);
        usleep(200000); // give the CPU 200ms of a break
        
    }

        return 0;
}
