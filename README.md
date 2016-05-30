# Drupal Star Export

This is a work in progress / proof of concept Drupal module that implements a drush command in order to export custom node content , field collections, field collection data (including multi value arrays).

The goal of this is to implement a polished, dynamic module that can accept arguments and options in order to specify custom content types, field collection names and groups of single and multi value fields in order to export and import the data to and from a CSV.

Details and a walk-through of the code can be found at https://www.shift8web.ca

## Command reference

### To import 
`drush star-import --f="test.csv"`

###To export 
`drush star-export > test.csv`
