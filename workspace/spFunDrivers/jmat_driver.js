// require jmat module
var jmat = require("/usr/local/lib/jmat/jmat.min.fpdiff.js");

// node file.js funcName douebleInput intInput inputPermu
var myArgs = process.argv.slice(2);

// readin input string
var funcName = myArgs[0]
var doubleStrings = myArgs[1];
var intStrings = myArgs[2];
var inputPermu = myArgs[3]
var douebleInput = doubleStrings.split(',').map(Number);
var intInput = intStrings.split(',').map(Number);

// convert input to a single
var n = inputPermu.length;
var doublePos = 0;
var intPos = 0;
var inputString = "";
for (i = 0; i < n; i++) {
    var c = inputPermu.charAt(i);
    if (c == 'd'){
        inputString+=douebleInput[doublePos++] + ",";
    }else{
        inputString+=intInput[intPos++] + ",";
    }
}
inputString = inputString.substring(0, inputString.length - 1);

// construct funcCall str
var funcCall = "jmat." + funcName + "("+ inputString + ")";

// output result to stdout
try {
    var func = eval(funcCall);
    if (func.im != 0) {
        console.log("Exception: Complex Number Returned. " + func.re + "re" + func.im + "i");
    }else{
        console.log(func.re);
    }   
}
catch(err) {// if not executable
    console.log(err);
}

