use json;
use . test;

like(json.stringify(3),  "3", "int");
like(json.stringify(3.14), "3.1400000000000001", "num");
like(json.stringify("hoge"), q!"hoge"!, "string");
like(json.stringify(q!"!), q!"\\u0034"!, "string");
like(json.stringify(q!🍣!), q!"\\u127843"!, "string");
like(json.stringify([]), q![]!, "empty array");
like(json.stringify([1,2,3]), q![1,2,3]!, "array");
like(json.stringify({}), q!{}!, "empty map");
like(json.stringify({a =>1}), q!{"a":1}!, "map");
done_testing()

