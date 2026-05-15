// Model Vector object with closures
function Vector(x, y) {
  function getX() {
    return x;
  }

  function getY() {
    return y;
  }

  function add(vector2) {
    return Vector(x + vector2("getX")(), y + vector2("getY")());
  }

  function dispatch(name) {
    if (name == "getX") {
      return getX;
    }
    if (name == "getY") {
      return getY;
    }
    if (name == "add") {
      return add;
    }
    console.error("unknown method");
  }

  return dispatch;
}

function printVector(name, vector) {
  console.log(`${name}: (${vector("getX")()}, ${vector("getY")()})`);
}

let vector1 = Vector(1, 2);
printVector("vector 1", vector1);

let vector2 = Vector(3, 4);
printVector("vector 2", vector2);

let vector3 = vector1("add")(vector2);
printVector("vector 3", vector3);
