# aesd-assignments
This repo contains public starter source code, scripts, and documentation for Advanced Embedded Software Development (ECEN-5713) and Advanced Embedded Linux Development assignments University of Colorado, Boulder.

## Setting Up Git

Use the instructions at [Setup Git](https://help.github.com/en/articles/set-up-git) to perform initial git setup steps. For AESD you will want to perform these steps inside your Linux host virtual or physical machine, since this is where you will be doing your development work.

## Setting up SSH keys

See instructions in [Setting-up-SSH-Access-To-your-Repo](https://github.com/cu-ecen-aeld/aesd-assignments/wiki/Setting-up-SSH-Access-To-your-Repo) for details.

## Specific Assignment Instructions

Some assignments require further setup to pull in example code or make other changes to your repository before starting.  In this case, see the github classroom assignment start instructions linked from the assignment document for details about how to use this repository.

## Testing

The basis of the automated test implementation for this repository comes from [https://github.com/cu-ecen-aeld/assignment-autotest/](https://github.com/cu-ecen-aeld/assignment-autotest/)

The assignment-autotest directory contains scripts useful for automated testing  Use
```
git submodule update --init --recursive
```
to synchronize after cloning and before starting each assignment, as discussed in the assignment instructions.

As a part of the assignment instructions, you will setup your assignment repo to perform automated testing using github actions.  See [this page](https://github.com/cu-ecen-aeld/aesd-assignments/wiki/Setting-up-Github-Actions) for details.

Note that the unit tests will fail on this repository, since assignments are not yet implemented.  That's your job :)


## STEPS TO MANUALLY TEST ASSIGNMENT 8 on your native machine

These steps are useful for debugging your application before attempting to run on
an embedded target
After following assignment implementation steps

- cd into your aesd-char-driver DIRECTORY and do a make to build for your development machine.
- RUN ./aesdchar_unload, followed by ./aesdchar_load to load your module on your development machine

From your main root assignment directory,

- RUN ./assignment-autotest/test/assignment8/drivertest.sh to verify you implementation

When drivertest succeeds, start your modified aesdsocket application from the server subdirectory
Verify it passes sockettest.sh by running ./assignment-autotest/test/assignment8/sockettest.sh

## STEPS TO MANUALLY TEST ASSIGNMENT 9 on your native machine

These steps are useful for debugging your application before attempting to run on
an embedded target
After following assignment implementation steps

cd into your aesd-char-driver DIRECTORY and do a make to build for your development machine.

RUN ./aesdchar_unload, followed by ./aesdchar_load to load your module on your development machine

From your main root assignment directory,

RUN ./assignment-autotest/test/assignment9/drivertest.sh to verify you implementation

When drivertest succeeds, start your modified aesdsocket application from the server subdirectory

Verify it passes sockettest.sh by running ./assignment-autotest/test/assignment9/sockettest.sh
Test of assignment assignment9 complete with success
