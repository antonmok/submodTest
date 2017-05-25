#pragma once

void TransformVector(double inVector[2], double inMatrix[6], double outVector[2]) {

    /*if(!inMatrix)
        return inVector;*/

    outVector[0] = inMatrix[0] * inVector[0] + inMatrix[2] * inVector[1] + inMatrix[4];
    outVector[1] = inMatrix[1] * inVector[0] + inMatrix[3] * inVector[1] + inMatrix[5];
}

/*inverseMatrix: function(inMatrix)
{
    if(!inMatrix)
        return inMatrix;

    var a = inMatrix[0];
    var b = inMatrix[1];
    var c = inMatrix[2];
    var d = inMatrix[3];
    var t1 = inMatrix[4];
    var t2 = inMatrix[5];
    var det = a*d-b*c;

    return [
      d/det,
      -b/det,
      -c/det,
      a/det,
      (c*t2-d*t1)/det,
      (b*t1-a*t2)/det
    ];

},

determinante : function(inMatrix)
{
    if(!inMatrix)
        return 1;
    return inMatrix[0]*inMatrix[3]-inMatrix[1]*inMatrix[2];
},*/

void MultiplyMatrix(double inMatrixA[6], double inMatrixB[6], double outMatrix[6])
{
    if (!inMatrixA) {
        outMatrix[0] = inMatrixB[0];
        outMatrix[1] = inMatrixB[1];
        outMatrix[2] = inMatrixB[2];
        outMatrix[3] = inMatrixB[3];
        outMatrix[4] = inMatrixB[4];
        outMatrix[5] = inMatrixB[5];
    }

    if (!inMatrixB) {
        outMatrix[0] = inMatrixA[0];
        outMatrix[1] = inMatrixA[1];
        outMatrix[2] = inMatrixA[2];
        outMatrix[3] = inMatrixA[3];
        outMatrix[4] = inMatrixA[4];
        outMatrix[5] = inMatrixA[5];
    }

    outMatrix[0] = inMatrixA[0]*inMatrixB[0] + inMatrixA[1]*inMatrixB[2];
    outMatrix[1] = inMatrixA[0]*inMatrixB[1] + inMatrixA[1]*inMatrixB[3];
    outMatrix[2] = inMatrixA[2]*inMatrixB[0] + inMatrixA[3]*inMatrixB[2];
    outMatrix[3] = inMatrixA[2]*inMatrixB[1] + inMatrixA[3]*inMatrixB[3];
    outMatrix[4] = inMatrixA[4]*inMatrixB[0] + inMatrixA[5]*inMatrixB[2] + inMatrixB[4];
    outMatrix[5] = inMatrixA[4]*inMatrixB[1] + inMatrixA[5]*inMatrixB[3] + inMatrixB[5];
}

void TransformBox(double inBox[4], double inMatrix[6], double outBox[4])
{
    /*if(!inMatrix)
        return inBox;*/

    double t[4][2];
    double tmp[2];

    tmp[0] = inBox[0];
    tmp[1] = inBox[1];
    TransformVector(tmp, inMatrix, t[0]);

    tmp[0] = inBox[0];
    tmp[1] = inBox[3];
    TransformVector(tmp, inMatrix, t[1]);

    tmp[0] = inBox[2];
    tmp[1] = inBox[3];
    TransformVector(tmp, inMatrix, t[2]);

    tmp[0] = inBox[2];
    tmp[1] = inBox[1];
    TransformVector(tmp, inMatrix, t[3]);

    double minX, minY, maxX, maxY;

    minX = maxX = t[0][0];
    minY = maxY = t[0][1];

    for (uint i = 1; i < 4; ++i) {
        if (minX > t[i][0])
            minX = t[i][0];

        if (maxX < t[i][0])
            maxX = t[i][0];

        if (minY > t[i][1])
            minY = t[i][1];

        if (maxY < t[i][1])
            maxY = t[i][1];
    }

    outBox[0] = minX;
    outBox[1] = minY;
    outBox[2] = maxX;
    outBox[3] = maxY;
}

