#include <bits/stdc++.h>

using namespace std;

const int maxN = 1000;
const int maxK = 12;

int tab[maxK][maxN];
pair<long long, long long> dp[maxN][1 << maxK];

// This function for a given column calculates
// best cover of such column with only vertical
// dominos taking into account that no collisions
// with a given mask should appear.
int bestVerticalCover(int col, int mask, int k)
{
    // Edge case
    if(k == 1) return 0;
    
    int dpCol[k]; // DP for best cover from 0 to i
    int prevBit; // Last considered bit
    
    // Setting first two maximized values manually.
    dpCol[0] = 0;
    prevBit = mask % 2;
    mask >>= 1;
    dpCol[1] = prevBit == 0 && mask % 2 == 0 ? max(tab[0][col] + tab[1][col], 0) : 0;
    prevBit = mask % 2;
    mask >>= 1;
    
    // DP for column
    for(int i = 2; i < k; i++)
    {
        if(mask % 2 == 1 || prevBit == 1)
        {
            dpCol[i] = dpCol[i-1];
            prevBit = mask % 2;
            mask >>= 1;
            continue;
        }
        
        dpCol[i] = max(dpCol[i-2] + tab[i-1][col] + tab[i][col], dpCol[i-1]);
        prevBit = mask % 2;
        mask >>= 1;
    }
    
    return dpCol[k-1];
}

// This function calculates a sum of horizontal
// dominos placement by a given mask.
int horizontalCover(int rightCol, int mask, int k)
{
    int sum = 0;
    
    for(int i = 0; i < k; i++)
    {
        int b = mask % 2;
        
        sum += b * (tab[i][rightCol] + tab[i][rightCol-1]);
        
        mask >>= 1;
    }
    
    return sum;
}

// A function for generating only desired
// masks for (n-1)-th column and given a certain mask
// in n-th column and maximizing result.
// patternMask is copy of mask (at first) to not lose info about mask
// buildedMask is mask builded till current function call
// kCounter means which bit of a mask the function is currently generating
// wasZero tells if last bit was equal 0
// vc is best vertical cover for a given mask and column col
// hc is horizontal cover for a given mask and column col (it's right column)
void genMasks(int col, int mask, int patternMask, int buildedMask, int kCounter, int k, bool wasZero, int vc, int hc)
{
    // If full mask was built.
    if(kCounter == k)
    {
        // After establishing both masks we take best result for buildedMask
        // for col-1 and best vertical cover for summed masks for col-1 and
        // sum of current horizontal cover given by mask and best vertical
        // cover for col, thus we get full new result.
        long long b0 = dp[col - 1][buildedMask].first + dp[col - 1][buildedMask + mask].second + hc + vc;
        
        // If new result is better than current best one.
        if(b0 > dp[col][mask].first + dp[col][mask].second)
        {
            // We don't subtract vertical cover from here col
            // as it shoult be in the second field
            dp[col][mask].first = b0 - vc;
            dp[col][mask].second = vc;
        }
        
        return;
    }
    
    // Adding bit = 0
    genMasks(col, mask, patternMask >> 1, buildedMask, kCounter + 1, k, true, vc, hc);
    
    // Adding bit = 1
    // Last two conditions come from observations
    if(patternMask % 2 == 0 && tab[kCounter][col-1] + tab[kCounter][col-2] > 0 && wasZero)
        genMasks(col, mask, patternMask >> 1, buildedMask + (1 << kCounter), kCounter + 1, k, false, vc, hc);
}

// Solving the problem
long long sol(int n, int k)
{
    // Edge case
    if(n == 1)
        return bestVerticalCover(0, 0, k);
    
    // Maximum mask + 1
    int endMask = 1 << k;
    
    // Initial maximized values for first 2 columns
    for(int mask = 0; mask < endMask; mask++)
    {
        int b0 = bestVerticalCover(0, mask, k);
        int b1 = bestVerticalCover(1, mask, k);
        
        dp[1][mask].first = b0 + horizontalCover(1, mask, k);
        dp[1][mask].second = b1;
    }
    
    // DP loop
    for(int i = 2; i < n; i++)
    {
        for(int mask = 0; mask < endMask; mask++)
        {
            // Dividing by 2 is for safety to avoid exceeding the scope
            // of long long when adding these two together.
            dp[i][mask] = {-LLONG_MAX / 2LL, -LLONG_MAX / 2LL};
            
            // To not calculate it in every recursive call.
            int vc = bestVerticalCover(i, mask, k);
            int hc = horizontalCover(i, mask, k);
            
            // Recursive masks generating and maximizing dp[i][mask]
            genMasks(i, mask, mask, 0, 0, k, true, vc, hc);
        }
    }
    
    // Reading best result
    long long maxim = -LLONG_MAX;
    
    for(int mask = 0; mask < endMask; mask++)
        maxim = max(maxim, dp[n-1][mask].first + dp[n-1][mask].second);
    
    return maxim;
}

int main()
{
    ios_base::sync_with_stdio(0);
    cin.tie(0);
    cout.tie(0);
    
    int n, k;
    
    cin >> n >> k;
    
    for(int i = 0; i < k; i++)
    {
        for(int j = 0; j < n; j++)
        {
            cin >> tab[i][j];
        }
    }
    
    cout << sol(n, k);
    
    return 0;
}