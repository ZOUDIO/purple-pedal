# Built environment pre-requisit
VSCode + Devcontainer plugin ( called 'Dev Containers', created by Microsoft )
WSL2

# Install WSL

Option 1)
go to Microsoft Store
install Ubunut 22.04.05 LTS

Option 2)
wsl --list --online
wsl --install -d Ubuntu-22.04

WARNING FOR <Deleted Link> IT MANAGED SYSTEMS

If you receive an error like below during fetching the docker image
0.882 ERROR: cannot verify objects.githubusercontent.com's certificate, issued b
y ‘CN=Cisco Umbrella Secondary SubCA ams-SG,O=Cisco’:
0.882   Unable to locally verify the issuer's authority.

Please disable the Umbrella services
```
services.msc
->tab->Standard
->Stop Cisco Secure Client - Umbrella Agent
->Stop Cisco Secure Client - Umbrella SWG Agent
run docker image ( during the first run all software will be fetched)
->Start Cisco Secure Client - Umbrella Agent
->Start Cisco Secure Client - Umbrella SWG Agent
```
# Opening the projet

## clone the project to WSL folder and open (Recommended in windows)
This method is recommended because the disk access is fast and thus build speed is much faster.
On Linux, this is not applicable and use the method described in next section.

This method is inspired by: https://code.visualstudio.com/blogs/2020/07/01/containers-wsl

Follow the below steps to clone code in WSL and open it in VSCode:

1. set up git and ssh keys on WSL2.
2. git clone the project to ~/Documents folder. Note that do not use any folder mapped from your WIndows host to WSL, because this will result in slow disk access and slow build.
3. enter the folder and start vscode from the folder
```shell
cd ~/Documents/purple_pedal
code .
```
4. VSCode will open the folder, set up the docker machine. Then proceed do section "Initialize the zephyr workspace"

# flash and debug the project

See application readme file for detailed build, flash and debug instructions

# Purple Pedal application

see /zephyr_sdk/app_purple_pedal for the Purple Pedal application.

