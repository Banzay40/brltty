/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

package org.a11y.brltty.core;

import java.util.AbstractQueue;
import java.util.concurrent.LinkedBlockingDeque;

public class CoreWrapper {
  public static native int construct (String[] arguments);
  public static native boolean update ();
  public static native void destruct ();

  public static native boolean changeLogLevel (String logLevel);
  public static native boolean changeTextTable (String textTable);
  public static native boolean changeAttributesTable (String attributesTable);
  public static native boolean changeContractionTable (String contractionTable);

  private static final AbstractQueue<Runnable> runQueue = new LinkedBlockingDeque<Runnable>();

  public static boolean runOnCoreThread (Runnable runnable) {
    return runQueue.offer(runnable);
  }

  public static void processRunQueue () {
    Runnable runnable;
    while ((runnable = runQueue.poll()) != null) runnable.run();
  }

  private static volatile boolean stop = false;

  public static void stop () {
    stop = true;
  }

  public static int run (String[] arguments) {
    int exitStatus = construct(arguments);

    if (exitStatus == ProgramExitStatus.SUCCESS.value) {
      while (update()) {
        if (stop) break;
        processRunQueue();
      }
    } else if (exitStatus == ProgramExitStatus.FORCE.value) {
      exitStatus = ProgramExitStatus.SUCCESS.value;
    }

    destruct();
    return exitStatus;
  }

  public static void main (String[] arguments) {
    System.exit(run(arguments));
  }

  static {
    System.loadLibrary("brltty_core");
    System.loadLibrary("brltty_jni");
  }
}
