#!/usr/bin/python
#-*- coding: utf-8 -*-
height = input('height : ')
weight = input('weight : ') 
BMI = weight // (height * height)
if BMI < 18.5:
   print'����'
elif BMI < 25: 
   print'����'
elif BMI < 28:
   print'����'
elif BMI < 32:
   print'����'
else: 
   print'���ط���'
 
