#include <repair.h>

repair_digram dr1("aaa bbb", 10);


private static final int FREQ1 = 10;
private static final int FREQ2 = 13;
private static final int FREQ3 = 13;
private static final int FREQ4 = 7;
private static final int FREQ5 = 5;

private static final String KEY1 = "aaa bbb";
private static final String KEY2 = "bbb ccc";
private static final String KEY3 = "ccc eee";
private static final String KEY4 = "eee fff";
private static final String KEY5 = "fff ggg";

private RepairDigramRecord dr1 = new RepairDigramRecord(KEY1, FREQ1);
private RepairDigramRecord dr2 = new RepairDigramRecord(KEY2, FREQ2);
private RepairDigramRecord dr3 = new RepairDigramRecord(KEY3, FREQ3);
private RepairDigramRecord dr4 = new RepairDigramRecord(KEY4, FREQ4);
private RepairDigramRecord dr5 = new RepairDigramRecord(KEY5, FREQ5);
