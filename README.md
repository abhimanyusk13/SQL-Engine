Client SQL →  [Connection Manager] 
               ↓
            [Parser] 
               ↓
        [Binder & Semantic Analyzer]
               ↓
       [Logical Plan Generator]
               ↓
         [Query Optimizer]
               ↓
      [Physical Plan Generator]
               ↓
         [Execution Engine]
               ↓
         [Storage Engine]
               ↓
         [Result Set ← Client]
