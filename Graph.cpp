

//#include "g2o/types/slam3d/types_six_dof_quat.h"
#include "g2o/core/graph_optimizer_sparse.h"
#include "g2o/core/block_solver.h"
#include "g2o/core/solver.h"
#include "g2o/solvers/cholmod/linear_solver_cholmod.h"
//#include "g2o/solvers/dense/linear_solver_dense.h"
//#include "g2o/types/sba/types_six_dof_expmap.h"
#include "g2o/math_groups/se3quat.h"
#include "g2o/core/structure_only_solver.h"

#include "g2o/types/slam3d/vertex_se3_quat.h"
#include "g2o/types/slam3d/edge_se3_quat.h"

#include "Graph.h"

Graph::Graph()
{
}

void Graph::initialize()
{
	_optimizer.setMethod(g2o::SparseOptimizer::LevenbergMarquardt);
	_optimizer.setVerbose(true);
	
	g2o::BlockSolver_6_3::LinearSolverType * linearSolver;
    linearSolver = new g2o::LinearSolverCholmod<g2o::BlockSolver_6_3::PoseMatrixType>();
    g2o::BlockSolver_6_3 * solver_ptr = new g2o::BlockSolver_6_3(&_optimizer,linearSolver);
    _optimizer.setSolver(solver_ptr);
}


void Graph::addVertex(int id, const Eigen::Matrix4f &transfo)
{
	g2o::VertexSE3 *vertexSE3 = NULL;
	
    Eigen::Matrix4f rot = Eigen::Matrix4f::Identity();
        
    /*rot(0,0) = 0;
    rot(0,2) = 1;
    rot(2,0) = -1;
    rot(2,2) = 0;*/
    		
    Eigen::Affine3f eigenTransform(rot * transfo);
    
    Eigen::Quaternionf eigenRotation(eigenTransform.rotation());
    
    g2o::SE3Quat poseSE3(
    		// NOTE the order of the arguments : w comes first!
			Eigen::Quaterniond(eigenRotation.w(), eigenRotation.x(), eigenRotation.y(), eigenRotation.z()),
			Eigen::Vector3d(eigenTransform(0,3), eigenTransform(1,3), eigenTransform(2,3)));
	
	// add new vertex to the graph
    vertexSE3 = new g2o::VertexSE3();
	vertexSE3->setId(id);
	vertexSE3->estimate() = poseSE3;
	
	if (id==0)
		vertexSE3->setFixed(true);
	else
		vertexSE3->setFixed(false);
    
	_optimizer.addVertex(vertexSE3);

	/*Eigen::Matrix4f mat = vertexSE3->estimate().to_homogenious_matrix().cast<float>();
	//Eigen::Matrix4f mat = pose.to_homogenious_matrix().cast<float>();
	// downcast to float
    std::cerr << "Camera mat SE3\n" << mat << std::endl;*/
}

void Graph::addEdge(int id1, int id2, const Transformation &transfo)
{
    //SE2 transf = transfo._matrix.inverse();
    
    Eigen::Matrix4f rot = Eigen::Matrix4f::Identity();
        
    /*rot(0,0) = 0;
    rot(0,2) = 1;
    rot(2,0) = -1;
    rot(2,2) = 0;*/
    		
    Eigen::Affine3f eigenTransform(rot * transfo._matrix);			
    Eigen::Quaternionf eigenRotation(eigenTransform.rotation());
    
    g2o::SE3Quat transfoSE3(
    		// NOTE the order of the arguments : w comes first!    		
    		Eigen::Quaterniond(eigenRotation.w(), eigenRotation.x(), eigenRotation.y(), eigenRotation.z()),
    		Eigen::Vector3d(eigenTransform(0,3), eigenTransform(1,3), eigenTransform(2,3)));
    
	// add new edge to the graph
    g2o::EdgeSE3* edgeSE3 = new g2o::EdgeSE3;
    edgeSE3->vertices()[0] = _optimizer.vertex(id1);
    edgeSE3->vertices()[1] = _optimizer.vertex(id2);
    edgeSE3->setMeasurement(transfoSE3.inverse());
    edgeSE3->setInverseMeasurement(transfoSE3);
    
    Eigen::Matrix<double, 6, 6, 0, 6, 6> mat;
    mat.setIdentity(6,6);
    edgeSE3->information() = mat;
    
    _optimizer.addEdge(edgeSE3);
}

bool Graph::getTransfo(int id, Transformation& transfo)
{
	g2o::VertexSE3 *vertexSE3;
	
   transfo._idOrig = -1;
	transfo._idDest = id;
	transfo._error = 0;
	transfo._matrix = Eigen::Matrix4f::Identity();
     
	vertexSE3 = (g2o::VertexSE3*)_optimizer.vertex(id);
	if (vertexSE3 != NULL)
	{
		Eigen::Matrix4d mat = vertexSE3->estimate().to_homogenious_matrix().inverse();
		// downcast to float
		transfo._matrix = mat.cast<float>();
		return true;
	}
	return false;
}

void Graph::optimize()
{
    _optimizer.initializeOptimization();
    _optimizer.setVerbose(true);
    _optimizer.optimize(2);
}

void Graph::save(const char* filename)
{
	// save the graph
	char buf_graph[256];
	sprintf(buf_graph, "%s/%s", _saveDirectory.c_str(), filename);
	std::ofstream foutopt(buf_graph);
    _optimizer.save(foutopt);	
}