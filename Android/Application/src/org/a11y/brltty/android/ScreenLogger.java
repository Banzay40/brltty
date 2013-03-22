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

package org.a11y.brltty.android;

import java.util.List;
import java.util.ArrayList;

import android.util.Log;

import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;

import android.graphics.Rect;

public class ScreenLogger {
  private final String logTag;

  private static final String ROOT_NODE_NAME = "root";

  public static String getRootNodeName () {
    return ROOT_NODE_NAME;
  }

  public final void log (String message) {
    Log.d(logTag, message);
  }

  public final void addValue (
    List<CharSequence> values,
    boolean condition,
    CharSequence onValue,
    CharSequence offValue
  ) {
    CharSequence value = condition? onValue: offValue;

    if (value != null) {
      values.add(value);
    }
  }

  public final void addValue (
    List<CharSequence> values,
    boolean condition,
    CharSequence onValue
  ) {
    addValue(values, condition, onValue, null);
  }

  public final void addValue (
    List<CharSequence> values,
    CharSequence value
  ) {
    addValue(values, true, value);
  }

  public final void logProperty (CharSequence name, CharSequence ... values) {
    if (values.length > 0) {
      StringBuilder sb = new StringBuilder();

      sb.append(name);
      sb.append(':');

      for (CharSequence value : values) {
        sb.append(' ');

        if (value == null) {
          sb.append("null");
        } else if (value.length() == 0) {
          sb.append("nil");
        } else {
          sb.append(value);
        }
      }

      log(sb.toString());
    }
  }

  public final void logProperty (CharSequence name, int value) {
    logProperty(name, Integer.toString(value));
  }

  public final void logNodeProperties (AccessibilityNodeInfo node) {
    {
      List<CharSequence> values = new ArrayList<CharSequence>();
      addValue(values, node.getParent()==null, ROOT_NODE_NAME);

      {
        int count = node.getChildCount();
        addValue(values, count>0, "cld=" + count);
      }

      addValue(values, node.isVisibleToUser(), "vis", "inv");
      addValue(values, node.isEnabled(), "enb", "dsb");
      addValue(values, node.isSelected(), "sel");
      addValue(values, node.isScrollable(), "scl");
      addValue(values, node.isFocusable(), "fcb");
      addValue(values, node.isFocused(), "fcd");
      addValue(values, node.isAccessibilityFocused(), "fca");
      addValue(values, node.isClickable(), "clk");
      addValue(values, node.isLongClickable(), "lng");
      addValue(values, node.isCheckable(), "ckb");
      addValue(values, node.isChecked(), "ckd");
      addValue(values, node.isPassword(), "pwd");
      logProperty("flgs", values.toArray(new CharSequence[values.size()]));
    }

    logProperty("desc", node.getContentDescription());
    logProperty("text", node.getText());
    logProperty("obj", node.getClassName());
    logProperty("app", node.getPackageName());

    {
      AccessibilityNodeInfo subnode = node.getLabelFor();

      if (subnode != null) {
        logProperty("lbl", ScreenUtilities.getNodeText(subnode));
      }
    }

    {
      AccessibilityNodeInfo subnode = node.getLabeledBy();

      if (subnode != null) {
        logProperty("lbd", ScreenUtilities.getNodeText(subnode));
      }
    }

    {
      Rect location = new Rect();
      node.getBoundsInScreen(location);
      logProperty("locn", location.toShortString());
    }
  }

  public final void logTreeProperties (
    AccessibilityNodeInfo root,
    CharSequence name
  ) {
    logProperty("name", name);

    if (root != null) {
      logNodeProperties(root);

      {
        int childCount = root.getChildCount();

        for (int childIndex=0; childIndex<childCount; childIndex+=1) {
          logTreeProperties(root.getChild(childIndex), name + "." + childIndex);
        }
      }
    }
  }

  public final void logTreeProperties (AccessibilityNodeInfo root) {
    logTreeProperties(root, ROOT_NODE_NAME);
  }

  public final void logNodeIdentity (AccessibilityNodeInfo node, String description) {
    StringBuilder sb = new StringBuilder();

    sb.append(description);
    sb.append(": ");

    if (node == null) {
      sb.append("null");
    } else {
      String text = ScreenUtilities.getNodeText(node);

      if ((text != null) && (text.length() > 0)) {
        sb.append(text);
      } else {
        sb.append(node.getClassName());
      }

      Rect location = new Rect();
      node.getBoundsInScreen(location);
      sb.append(' ');
      sb.append(location.toShortString());
    }

    log(sb.toString());
  }

  public final void logEvent (AccessibilityEvent event) {
    logProperty("accessibility event", event.toString());

    AccessibilityNodeInfo node = event.getSource();

    if (node != null) {
      logProperty("current window", node.getWindowId());
    }

    logNodeIdentity(node, "event node");
    logTreeProperties(ScreenUtilities.findRootNode(node));
  }

  public ScreenLogger (String tag) {
    logTag = tag;
  }
}
