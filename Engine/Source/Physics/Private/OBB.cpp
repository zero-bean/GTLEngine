#include "pch.h"
#include "Physics/Public/OBB.h"

bool FOBB::OverlapsAABB(const FAABB& InAABB) const
{
	constexpr float kEps = 1e-6f;

	const FVector aabbCenter = (InAABB.Min + InAABB.Max) * 0.5f;
	const FVector aabbExt    = (InAABB.Max - InAABB.Min) * 0.5f; // (ex, ey, ez)

	// World-space cardinal axes for the AABB
	const FVector BAxis[3] = {
		FVector(1.f, 0.f, 0.f),  // X
		FVector(0.f, 1.f, 0.f),  // Y
		FVector(0.f, 0.f, 1.f)   // Z
	};

	const FVector& ACenter = Center;     // OBB center
	const FVector& AE      = Extents;    // OBB half-extents: (ax, ay, az)
	const FVector AAxis[3]   = {
		FVector(Orientation.Data[0][0], Orientation.Data[0][1], Orientation.Data[0][2]),
		FVector(Orientation.Data[1][0], Orientation.Data[1][1], Orientation.Data[1][2]),
		FVector(Orientation.Data[2][0], Orientation.Data[2][1], Orientation.Data[2][2])
	};

	float R[3][3];
	float AbsR[3][3];
	for (int i = 0; i < 3; ++i)
	{
		R[i][0] = AAxis[i].Dot(BAxis[0]);
		R[i][1] = AAxis[i].Dot(BAxis[1]);
		R[i][2] = AAxis[i].Dot(BAxis[2]);

		AbsR[i][0] = fabs(R[i][0]) + kEps;
		AbsR[i][1] = fabs(R[i][1]) + kEps;
		AbsR[i][2] = fabs(R[i][2]) + kEps;
	}

	const FVector tWorld = aabbCenter - ACenter;

	// Express t in OBB’s local frame: t' = [ dot(t,U0), dot(t,U1), dot(t,U2) ]
	float tA[3] = {
		tWorld.Dot(AAxis[0]),
		tWorld.Dot(AAxis[1]),
		tWorld.Dot(AAxis[2])
	};

	// Convenience accessors for components
	const float& ex = aabbExt.X;  // AABB half-extents
	const float& ey = aabbExt.Y;
	const float& ez = aabbExt.Z;

	const float& ax = AE.X;       // OBB half-extents
	const float& ay = AE.Y;
	const float& az = AE.Z;

// ------------------------------------------------------------------------
    // 1) Test the 3 OBB axes:  Ui
    //    |t' i| <= ra + rb, where:
    //    ra = Ai (OBB half-extent along Ui)
    //    rb = ex*|R[i][0]| + ey*|R[i][1]| + ez*|R[i][2]|
    // ------------------------------------------------------------------------
    {
        float ra, rb;
        // U0
        ra = ax;
        rb = ex * AbsR[0][0] + ey * AbsR[0][1] + ez * AbsR[0][2];
        if (fabs(tA[0]) > ra + rb) return false;

        // U1
        ra = ay;
        rb = ex * AbsR[1][0] + ey * AbsR[1][1] + ez * AbsR[1][2];
        if (fabs(tA[1]) > ra + rb) return false;

        // U2
        ra = az;
        rb = ex * AbsR[2][0] + ey * AbsR[2][1] + ez * AbsR[2][2];
        if (fabs(tA[2]) > ra + rb) return false;
    }

    // ------------------------------------------------------------------------
    // 2) Test the 3 AABB axes: Vj (world X, Y, Z)
    //    |t • Vj| <= ra + rb, where:
    //    ra = ax*|R[0][j]| + ay*|R[1][j]| + az*|R[2][j]|
    //    rb = AABB half-extent on axis j (ex/ey/ez)
    // ------------------------------------------------------------------------
    {
        float ra, rb, tProj;

        // Vx
        ra   = ax * AbsR[0][0] + ay * AbsR[1][0] + az * AbsR[2][0];
        rb   = ex;
        tProj = fabs(tWorld.X); // dot(tWorld, (1,0,0))
        if (tProj > ra + rb) return false;

        // Vy
        ra   = ax * AbsR[0][1] + ay * AbsR[1][1] + az * AbsR[2][1];
        rb   = ey;
        tProj = fabs(tWorld.Y); // dot(tWorld, (0,1,0))
        if (tProj > ra + rb) return false;

        // Vz
        ra   = ax * AbsR[0][2] + ay * AbsR[1][2] + az * AbsR[2][2];
        rb   = ez;
        tProj = fabs(tWorld.Z); // dot(tWorld, (0,0,1))
        if (tProj > ra + rb) return false;
    }

    // ------------------------------------------------------------------------
    // 3) Test the 9 cross-product axes: Ui x Vj
    //
    // Optimized projections (Gottschalk, OBBTree):
    // t_proj = | tA[(i+2)%3] * R[(i+1)%3][j] - tA[(i+1)%3] * R[(i+2)%3][j] |
    // ra     = A[(i+1)%3]*AbsR[(i+2)%3][j] + A[(i+2)%3]*AbsR[(i+1)%3][j]
    // rb     = B[(j+1)%3]*AbsR[i][(j+2)%3] + B[(j+2)%3]*AbsR[i][(j+1)%3]
    // ------------------------------------------------------------------------
    auto axisTest = [&](int i, int j, float Ai, float Aj, float Bx, float By) -> bool {
        const int i1 = (i + 1) % 3;
        const int i2 = (i + 2) % 3;
        const int j1 = (j + 1) % 3;
        const int j2 = (j + 2) % 3;

        const float tProj = fabs(tA[i2] * R[i1][j] - tA[i1] * R[i2][j]);
        const float ra    = Aj * AbsR[i2][j] + Ai * AbsR[i1][j];
        const float rb    = Bx * AbsR[i][j2] + By * AbsR[i][j1];

        return tProj <= (ra + rb);
    };

    // Map (Ai,Aj) and (Bx,By) per axis pair
    // A = (ax, ay, az), B = (ex, ey, ez)
    // For each (i,j), Ai = A[(i+1)%3], Aj = A[(i+2)%3], Bx = B[(j+1)%3], By = B[(j+2)%3].
    const float Aarr[3] = { ax, ay, az };
    const float Barr[3] = { ex, ey, ez };

    for (int i = 0; i < 3; ++i)
    {
        const float Ai = Aarr[(i + 1) % 3];
        const float Aj = Aarr[(i + 2) % 3];

        for (int j = 0; j < 3; ++j)
        {
            const float Bx = Barr[(j + 1) % 3];
            const float By = Barr[(j + 2) % 3];

            if (!axisTest(i, j, Ai, Aj, Bx, By))
                return false; // Found a separating axis
        }
    }

    // If none of the 15 axes separate, they overlap.
    return true;
}
