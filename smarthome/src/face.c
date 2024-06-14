#include <Python.h>
#define WGET_CMD "wget http://127.0.0.1:8080/?action=snapshot -O /tmp/SearchFace.jpg"
#define SEARCHFACE_FILE "/tmp/SearchFace.jpg"

void face_init(){
	
	//初始化
	Py_Initialize();

	//将当前路径添加到sys.path中
	PyObject *sys = PyImport_ImportModule("sys");
	PyObject *path = PyObject_GetAttrString(sys,"path");
	PyList_Append(path,PyUnicode_FromString("."));
}

void face_final(){
	
	Py_Finalize();
}

double face_category(){
	
	double result = 0.0;
	system(WGET_CMD);
	if(0 != access(SEARCHFACE_FILE,F_OK)){
		return result;
	}
	//导入face模块
	PyObject *pModule = PyImport_ImportModule("face");
	if (!pModule)
	{
		PyErr_Print();
		printf("ERROR: failed to load face.py\n");
		goto FAILED_MODULE;
	}

	// 获取alibaba_face函数对象
	PyObject *pFunc = PyObject_GetAttrString(pModule,"alibaba_face");
	if (!pFunc || !PyCallable_Check(pFunc))
	{
		PyErr_Print();
		printf("ERROR: function alibaba_face not found or not callable\n");
		goto FAILED_FUNC;
	}
	//传入参数
	//char *category = "comedy";
	//PyObject *pArgs  = Py_BuildValue("(s)",category);


	// 调用alibaba_face函数并获取返回值
	PyObject *pValue = PyObject_CallObject(pFunc, NULL);
	if (!pValue)
	{
		PyErr_Print();
		printf("ERROR: function call failed\n");
		goto FAILED_VALUE;
	}
	//将返回值转换为C类型
	
	if (!PyArg_Parse(pValue, "d", &result))
	{
		PyErr_Print();
		printf("Error: parse failed\n");
		goto FAILED_RESULT;
	}

	//打印返回值
	printf("pValue=%0.2lf\n", result);
	
FAILED_RESULT:
	Py_DECREF(pValue);
FAILED_VALUE:
	Py_DECREF(pFunc);
FAILED_FUNC:
	Py_DECREF(pModule);
FAILED_MODULE:
	// 关闭Python解释器
	//Py_Finalize();
	return result;
}


