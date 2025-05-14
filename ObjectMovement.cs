using System.Collections;
using System.Collections.Generic;
using Unity.VisualScripting;
using UnityEngine;

public static class ObjectMovement
{
    public static bool LinearMoveTowards(Transform obj, Vector3 target, float maxDelta)
    {
        obj.position = Vector3.MoveTowards(obj.position, target, maxDelta * Time.deltaTime);

        return obj.position == target;
    }

    public static bool LinearRotateTowards(Transform obj, Quaternion target, float maxDelta)
    {
        obj.rotation = Quaternion.RotateTowards(obj.rotation, target, maxDelta * Time.deltaTime);

        return Quaternion.Angle(obj.rotation, target) == 0f;
    }

    // move and rotation will at most not be time synchronized (!)
    public static bool LinearMoveAndRotateTowards(Transform obj, Vector3 targetV, Quaternion targetQ, float maxMoveDelta, float maxRotateDelta)
    {
        bool b1 = LinearMoveTowards(obj, targetV, maxMoveDelta);
        bool b2 = LinearRotateTowards(obj, targetQ, maxRotateDelta);

        return b1 && b2;
    }

    public static bool AcceleratedMoveTowards(Transform obj, Vector3 startV, Vector3 targetV, float maxDelta)
    {
        float fullDistance = Vector3.Magnitude(targetV - startV);
        float distanceDone = Vector3.Magnitude(obj.position - startV);

        float progress = distanceDone / fullDistance;

        float a = 3f * ParabolicFunc2(progress) + 0.05f;

        obj.position = Vector3.MoveTowards(obj.transform.position, targetV, a * maxDelta * Time.deltaTime);

        return obj.position == targetV;
    }

    public static bool AcceleratedRotateTowards(Transform obj, Quaternion startQ, Quaternion targetQ, float maxDelta)
    {
        float fullAngle = Quaternion.Angle(startQ, targetQ);
        float angleDone = Quaternion.Angle(startQ, obj.rotation);

        float progress = angleDone / fullAngle;

        float a = 3f * ParabolicFunc2(progress) + 0.05f;

        obj.rotation = Quaternion.RotateTowards(obj.rotation, targetQ, a * maxDelta * Time.deltaTime);

        return Quaternion.Angle(obj.rotation, targetQ) == 0f;
    }

    // move and rotation will at most not be time synchronized (!)
    public static bool AcceleratedMoveAndRotateTowards(Transform obj, Vector3 startV, Vector3 targetV, Quaternion startQ, Quaternion targetQ, float maxMoveDelta, float maxRotateDelta)
    {
        bool b1 = AcceleratedMoveTowards(obj, startV, targetV, maxMoveDelta);
        bool b2 = AcceleratedRotateTowards(obj, startQ, targetQ, maxRotateDelta);

        return b1 && b2;
    }

    public static bool TimedLinearMoveTowards(Transform obj, Vector3 start, Vector3 target, float animationTime)
    {
        float fullDistance = Vector3.Magnitude(target - start);
        float distanceDone = Vector3.Magnitude(obj.position - start);

        float progress = distanceDone / fullDistance;

        obj.position = Vector3.MoveTowards(obj.position, target, fullDistance / animationTime * Time.deltaTime);

        if (Vector3.SqrMagnitude(target - obj.position) < 0.00001f)
        {
            obj.position = target;
            return true;
        }
        else
        {
            return false;
        }
    }

    public static bool TimedLinearRotateTowards(Transform obj, Quaternion start, Quaternion target, float animationTime)
    {
        float fullAngle = Quaternion.Angle(start, target);

        obj.rotation = Quaternion.RotateTowards(obj.rotation, target, fullAngle / animationTime * Time.deltaTime);

        if (Quaternion.Angle(obj.rotation, target) < 0.05f)
        {
            obj.rotation = target;
            return true;
        }
        else
        {
            return false;
        }
    }

    public static bool TimedLinearMoveAndRotateTowards(Transform obj, Vector3 startV, Vector3 targetV, Quaternion startQ, Quaternion targetQ, float animationTime)
    {
        bool b1 = TimedLinearMoveTowards(obj, startV, targetV, animationTime);
        bool b2 = TimedLinearRotateTowards(obj, startQ, targetQ, animationTime);

        return b1 && b2;
    }

    public static bool TimedAcceleratedMoveTowards(Transform obj, Vector3 startV, Vector3 targetV, float startTime, float animationTime)
    {
        float progress = (Time.time - startTime) / animationTime;
        progress = Mathf.Clamp01(progress);

        obj.position = Vector3.Lerp(startV, targetV, SlowingFunc(progress));

        return obj.position == targetV;
    }

    public static bool TimedAcceleratedRotateTowards(Transform obj, Quaternion startQ, Quaternion targetQ, float startTime, float animationTime)
    {
        float progress = (Time.time - startTime) / animationTime;
        progress = Mathf.Clamp01(progress);

        obj.rotation = Quaternion.Lerp(startQ, targetQ, SlowingFunc(progress));

        return Quaternion.Angle(obj.rotation, targetQ) == 0f;
    }

    // move and rotation will at most not be time synchronized (!)
    public static bool TimedAcceleratedMoveAndRotateTowards(Transform obj, Vector3 startV, Vector3 targetV, Quaternion startQ, Quaternion targetQ, float startTime, float animationTime)
    {
        bool b1 = TimedAcceleratedMoveTowards(obj, startV, targetV, startTime, animationTime);
        bool b2 = TimedAcceleratedRotateTowards(obj, startQ, targetQ, startTime, animationTime);

        return b1 && b2;
    }

    // Do not call it when start.y and target.y are equal.
    public static bool ParabolicMoveTowards(Transform obj, Vector3 start, Vector3 target, float animationTime)
    {
        Vector3 flatPos = new Vector3(obj.position.x, 0f, obj.position.z);
        Vector3 startFlatPos = new Vector3(start.x, 0f, start.z);
        Vector3 targetFlatPos = new Vector3(target.x, 0f, target.z);

        // Remember you can do SqrMagnitude here to make an animation more "asymmetrical"
        float fullDistance = Vector3.Magnitude(targetFlatPos - startFlatPos);
        float distanceDone = Vector3.Magnitude(flatPos - startFlatPos);

        float progress = distanceDone / fullDistance;

        float basicY = start.y + (target.y - start.y) * progress;
        float newY = ParabolicFunc(progress);

        obj.position = Vector3.MoveTowards(flatPos, targetFlatPos, fullDistance / animationTime * Time.deltaTime);
        obj.position += new Vector3(0f, basicY + newY, 0f);

        if (Vector3.SqrMagnitude(target - obj.position) < 0.00001f)
        {
            obj.position = target;
            return true;
        }
        else
        {
            return false;
        }
    }

    // Use this when startPos.y and targetPos.y are the same.
    public static bool JumpAndRotateTowards(Transform obj, Vector3 startV, Quaternion startQ, Quaternion targetQ, float animationTime)
    {
        float fullAngle = Quaternion.Angle(startQ, targetQ);
        float angleDone = Quaternion.Angle(startQ, obj.rotation);

        float newY = ParabolicFunc(angleDone / fullAngle);

        obj.position = startV + Vector3.up * newY;
        obj.rotation = Quaternion.RotateTowards(obj.rotation, targetQ, fullAngle / animationTime * Time.deltaTime);

        if (Quaternion.Angle(obj.rotation, targetQ) < 0.02f)
        {
            obj.position = startV;
            obj.rotation = targetQ;
            return true;
        }
        else
        {
            return false;
        }
    }

    private static float ParabolicFunc(float x)
    {
        return -x * (x - 1f);
    }

    private static float ParabolicFunc2(float x)
    {
        return -(x - 0.5f) * (x - 0.5f) + 0.25f;
    }

    private static float LeftShiftedParabolicFunc(float x)
    {
        float p = Mathf.Pow(x - 1, 3);
        return -p * (p + 1f);
    }

    private static float SlowingFunc(float x)
    {
        return Mathf.Sin(0.5f * Mathf.PI * x);
    }

    /*
    private static float WaveFunc(float x, float a)
    {
        return Mathf.Atan(a * (2f * x - 1f)) * 0.5f / Mathf.Atan(a) + 0.5f;
    }
    */
}
